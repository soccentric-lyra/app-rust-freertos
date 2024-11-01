/**
 * @file    platform.c
 * @brief   C platform layer for the Rust application.
 * @details The C boundary to the HAL, FreeRTOS and the BOOT contract, with no
 *          application policy. src/platform/plat.rs must match platform.h.
 * @author  Sandesh Ghimire <sandesh@soccentric.com>
 * @date    2026-11-09
 * @target  NUCLEO-F767ZI (STM32F767ZIT6)
 * @srs     SRS-RFREERTOS-F767 (RFCON-004, RFBT-002, RFBT-005, RFRT-004,
 *          RFGEN-004, RFDBG-001, RFN-003)
 */

#include "stm32f7xx_hal.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include <stdint.h>
#include <string.h>

#include "app_board.h"
#include "app_fault.h"
#include "app_log.h"
#include "boot_contract.h"
#include "flash_confirm.h"
#include "handover.h"
#include "iwdg.h"
#include "platform.h"
#include "reset_cause.h"
#include "uart3.h"

/** Task bound by plat_button_bind(); written once by Rust, read by the ISR. */
static TaskHandle_t s_button_task;
/**
 * One-slot RX wake queue. Not a task notification: app_uart3_send() uses the
 * Comms task's notification for TX completion, and sharing it would end a
 * transmit early.
 */
static QueueHandle_t s_uart_rx_queue;

/* --- GPIO, button and EXTI ------------------------------------------------ */

/**
 * @brief Configure LD1..LD3 (PB0/PB7/PB14) off and B1 (PC13) on EXTI, both edges.
 * @details Used by main.c and, through plat_gpio_init(), by Rust.
 */
void app_gpio_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();

    /* Push-pull, active high; all three start off. */
    gpio.Pin       = APP_LED1_PIN | APP_LED2_PIN | APP_LED3_PIN;
    gpio.Mode      = GPIO_MODE_OUTPUT_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_LOW;
    gpio.Alternate = 0U;
    HAL_GPIO_Init(APP_LED1_PORT, &gpio);
    APP_LED1_PORT->BSRR = ((uint32_t)(APP_LED1_PIN | APP_LED2_PIN | APP_LED3_PIN)) << 16U;

    /* B1 is high when pressed. Both edges wake the Button task; Rust
     * debounces. */
    gpio.Pin       = APP_BTN1_PIN;
    gpio.Mode      = GPIO_MODE_IT_RISING_FALLING;
    gpio.Pull      = GPIO_PULLDOWN;
    gpio.Speed     = GPIO_SPEED_FREQ_LOW;
    gpio.Alternate = 0U;
    HAL_GPIO_Init(APP_BTN1_PORT, &gpio);
    __HAL_GPIO_EXTI_CLEAR_IT(APP_BTN1_PIN);

    /* The ISR calls a FromISR API, so its priority number must be >=
     * configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (5). */
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 6U, 0U);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

bool plat_gpio_init(void)
{
    app_gpio_init();
    return true;
}

void plat_led_set(uint32_t led, bool on)
{
    uint32_t pin = 0U;

    switch(led)
    {
    case 0U: pin = APP_LED1_PIN; break;
    case 1U: pin = APP_LED2_PIN; break;
    case 2U: pin = APP_LED3_PIN; break;
    default: return;
    }

    APP_LED1_PORT->BSRR = on ? pin : (pin << 16U);
}

bool plat_button_pressed(void) { return (APP_BTN1_PORT->IDR & APP_BTN1_PIN) != 0U; }

void plat_button_bind(void) { s_button_task = xTaskGetCurrentTaskHandle(); }

void plat_button_wait(uint32_t timeout_ms)
{
    (void)ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(timeout_ms));
}

/**
 * @brief EXTI 10..15 ISR: notify the Button task on a B1 (line 13) edge.
 * @details Only a notification is posted; the task reads the pin after waking,
 *          so no edge is lost.
 */
void EXTI15_10_IRQHandler(void)
{
    BaseType_t woken = pdFALSE;

    if((EXTI->PR & APP_BTN1_PIN) != 0U)
    {
        EXTI->PR = APP_BTN1_PIN;
        if(s_button_task != NULL)
        {
            vTaskNotifyGiveFromISR(s_button_task, &woken);
        }
        portYIELD_FROM_ISR(woken);
    }
}

/* --- USART3 HWLINK transport -------------------------------------------- */

bool plat_uart_init(void)
{
    if(s_uart_rx_queue == NULL)
    {
        s_uart_rx_queue = xQueueCreate(1U, sizeof(uint8_t));
        if(s_uart_rx_queue == NULL)
        {
            return false;
        }
    }
    return app_uart3_init();
}

bool plat_uart_set_baud(uint32_t baud) { return app_uart3_set_baud(baud); }

uint32_t plat_uart_read(uint8_t *dst, uint32_t max_len)
{
    return app_uart3_rx_read(dst, max_len);
}

bool plat_uart_rx_running(void) { return app_uart3_rx_running(); }

bool plat_uart_send(const uint8_t *data, uint32_t len, uint32_t timeout_ms)
{
    uint32_t count = len > UINT16_MAX ? UINT16_MAX : len;

    return app_uart3_send(data, (uint16_t)count, timeout_ms);
}

void plat_uart_bind(void) { /* RX wakes through s_uart_rx_queue; nothing to bind. */ }

void plat_uart_wait(uint32_t timeout_ms)
{
    uint8_t token = 0U;

    if(s_uart_rx_queue == NULL)
    {
        vTaskDelay(pdMS_TO_TICKS(timeout_ms == 0U ? 1U : timeout_ms));
        return;
    }
    (void)xQueueReceive(s_uart_rx_queue, &token, pdMS_TO_TICKS(timeout_ms));
}

void comms_post_rx_from_isr(void)
{
    uint8_t    token = 1U;
    BaseType_t woken = pdFALSE;

    if(s_uart_rx_queue != NULL)
    {
        (void)xQueueSendFromISR(s_uart_rx_queue, &token, &woken);
    }
    portYIELD_FROM_ISR(woken);
}

/* --- Watchdog ----------------------------------------------------------- */

bool plat_wdt_init(void)
{
    app_iwdg_init();
    return true;
}

void plat_wdt_feed(void) { app_iwdg_kick(); }

/* --- Debug log (USART6 through app_log.c) ------------------------------- */

void plat_log(uint8_t level, const char *text, uint32_t len)
{
    char     buf[PLAT_LOG_TEXT_MAX + 1U];
    uint32_t count = len > PLAT_LOG_TEXT_MAX ? PLAT_LOG_TEXT_MAX : len;

    if(text == NULL)
    {
        return;
    }
    memcpy(buf, text, count);
    buf[count] = '\0';
    app_log((char)level, "%s", buf);
}

/* --- Fault record ------------------------------------------------------- */

void plat_fault_boot_start(void) { app_fault_boot_start(); }

bool plat_fault_get(plat_fault_t *out)
{
    if(out == NULL || !app_fault_is_valid())
    {
        return false;
    }

    /* Magic and boot id stay in C. */
    out->reason = g_app_fault.reason;
    out->cfsr   = g_app_fault.cfsr;
    out->hfsr   = g_app_fault.hfsr;
    out->mmfar  = g_app_fault.mmfar;
    out->bfar   = g_app_fault.bfar;
    out->pc     = g_app_fault.pc;
    out->lr     = g_app_fault.lr;
    out->psr    = g_app_fault.psr;
    out->line   = g_app_fault.line;
    memcpy(out->task, g_app_fault.task, PLAT_FAULT_TASK_LEN);
    memcpy(out->file, g_app_fault.file, PLAT_FAULT_FILE_LEN);
    return true;
}

bool plat_fault_is_current_boot(void) { return app_fault_is_current_boot(); }

bool plat_fault_is_previous_boot(void) { return app_fault_is_previous_boot(); }

void plat_fault_clear(void) { app_fault_clear(); }

void plat_fatal(void) { app_fault_fatal(APP_FAULT_ASSERT, "rust", "panic", 0U); }

/* --- System ------------------------------------------------------------- */

uint32_t plat_vtor(void) { return SCB->VTOR; }

void plat_reboot(void)
{
    app_log_flush(50U);
    NVIC_SystemReset();
    for(;;)
    {
        __NOP();
    }
}

void plat_dcache_invalidate(uintptr_t addr, uint32_t len)
{
    if(addr == 0U || len == 0U)
    {
        return;
    }
    SCB_InvalidateDCache_by_Addr((uint32_t *)addr, (int32_t)len);
}

void plat_thread_name(const char *name)
{
    /* Task names are fixed by main.c. */
    (void)name;
}

uint32_t plat_stack_unused(void)
{
    if(xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)
    {
        return 0U;
    }
    return (uint32_t)uxTaskGetStackHighWaterMark(NULL) * (uint32_t)sizeof(StackType_t);
}

uint32_t plat_reset_cause_flags(void) { return app_reset_cause_flags; }

uint8_t plat_handover_slot(void) { return g_handover.slot; }

uint8_t plat_handover_trial(void) { return g_handover.trial; }

uint8_t plat_handover_trial_count(void) { return g_handover.trial_count; }

void plat_enter_bootloader(void)
{
    app_handover_request_bootloader();
    app_log_flush(50U);
    NVIC_SystemReset();
    for(;;)
    {
        __NOP();
    }
}

/* --- Time --------------------------------------------------------------- */

uint32_t plat_now_ms(void)
{
    /* Before the scheduler, use the TIM6 HAL tick (hal_timebase.c). */
    if(xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)
    {
        return HAL_GetTick();
    }
    return (uint32_t)(xTaskGetTickCount() * (TickType_t)portTICK_PERIOD_MS);
}

void plat_sleep_ms(uint32_t ms)
{
    TickType_t ticks = 1U;

    if(xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)
    {
        HAL_Delay(ms);
        return;
    }

    ticks = pdMS_TO_TICKS(ms == 0U ? 1U : ms);
    if(ticks == 0U)
    {
        ticks = 1U;
    }
    vTaskDelay(ticks);
}

/* --- CONFIRMED record of the running slot ------------------------------- */

bool plat_confirmed_read(uint8_t *out32)
{
    if(out32 == NULL)
    {
        return false;
    }

    /* The D-cache may still hold the erased sector from before the write. */
    SCB_InvalidateDCache_by_Addr((uint32_t *)(uintptr_t)APP_CONFIRMED_ADDR,
                                 (int32_t)APP_META_RECORD_SIZE);
    memcpy(out32, (const void *)(uintptr_t)APP_CONFIRMED_ADDR, APP_META_RECORD_SIZE);
    return true;
}

bool plat_confirm_running_slot(void) { return app_flash_confirm_running_slot(); }

/* --- critical-section shim ---------------------------------------------- */

/**
 * @brief critical-section 1.x acquire: save PRIMASK and disable IRQs.
 * @details cortex-m's "critical-section-single-core" exports are dropped by LTO
 *          because the crate is otherwise unreferenced, so this single-core
 *          implementation is provided weak; a Rust one still wins.
 * @return Previous PRIMASK, passed back to the release.
 */
__attribute__((weak)) uint32_t _critical_section_1_0_acquire(void)
{
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    return primask;
}

/**
 * @brief critical-section 1.x release: restore PRIMASK.
 * @param[in] restore_state Value returned by the matching acquire.
 */
__attribute__((weak)) void _critical_section_1_0_release(uint32_t restore_state)
{
    __set_PRIMASK(restore_state);
}
