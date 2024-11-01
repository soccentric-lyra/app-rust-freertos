/**
 * @file    app_fault.c
 * @brief   No-init fault record and fatal reset paths.
 * @details The record is written before the reset and reported in Status
 *          afterwards. The fatal path works before the HAL and with interrupts
 *          disabled, so it drives LD3 with register accesses only.
 * @author  Sandesh Ghimire <sandesh@soccentric.com>
 * @date    2026-11-03
 * @target  STM32F767ZIT6
 * @srs     DGEN-004, DRT-002, LED-005
 */

#include "stm32f7xx_hal.h"
#include "app_board.h"
#include "app_fault.h"
#include "boot_contract.h"

app_fault_record_t g_app_fault __attribute__((section(".noinit")));
/** Boot counter in .noinit; incremented once per boot, never 0. */
static uint32_t app_boot_id __attribute__((section(".noinit")));

/* A-9: record and boot id must fit the window BOOT leaves alone; the linker
 * script places .noinit at APP_NOINIT_ADDR. */
_Static_assert(sizeof(app_fault_record_t) + sizeof(uint32_t) <= APP_NOINIT_SIZE,
               "A-9: fault record exceeds the reserved no-init window");

/** Status names indexed by @ref app_fault_reason_t. */
static const char *const fault_reason_names[] = {
    "none",   "hard_fault",     "mem_manage",    "bus_fault", "usage_fault",
    "assert", "stack_overflow", "malloc_failed", "startup"};

/**
 * @brief Copy the basename of @p src into a fixed field, NUL-terminated.
 * @param[out] dst      Destination field.
 * @param[in]  dst_size Size of @p dst in bytes; 0 writes nothing.
 * @param[in]  src      Source path or NULL (stores "").
 */
static void fault_copy(char *dst, uint32_t dst_size, const char *src)
{
    uint32_t i = 0U;

    if(dst_size == 0U)
    {
        return;
    }
    if(src != NULL)
    {
        /* __FILE__ may be an absolute path; keep the basename so the
         * 31-character field identifies the file (DGEN-004). */
        const char *base = src;
        for(const char *p = src; *p != '\0'; p++)
        {
            if(*p == '/' || *p == '\\')
            {
                base = p + 1;
            }
        }
        while((i + 1U) < dst_size && base[i] != '\0')
        {
            dst[i] = base[i];
            i++;
        }
    }
    dst[i] = '\0';
}

void app_fault_boot_start(void)
{
    app_boot_id++;
    if(app_boot_id == 0U)
    {
        app_boot_id = 1U;
    }
}

bool app_fault_is_valid(void)
{
    return g_app_fault.magic == APP_FAULT_MAGIC &&
           g_app_fault.reason <= (uint32_t)APP_FAULT_STARTUP;
}

bool app_fault_is_current_boot(void)
{
    return app_fault_is_valid() && g_app_fault.boot_id == app_boot_id;
}

bool app_fault_is_previous_boot(void)
{
    uint32_t previous = app_boot_id - 1U;
    if(previous == 0U)
    {
        previous = 0xFFFFFFFFUL;
    }
    return app_fault_is_valid() && g_app_fault.boot_id == previous;
}

const char *app_fault_reason_text(app_fault_reason_t reason)
{
    if((uint32_t)reason >= (sizeof(fault_reason_names) / sizeof(fault_reason_names[0])))
    {
        return "unknown";
    }
    return fault_reason_names[reason];
}

const char *app_fault_task(void) { return g_app_fault.task; }

const char *app_fault_file(void) { return g_app_fault.file; }

void app_fault_save(app_fault_reason_t reason, uint32_t cfsr, uint32_t hfsr,
                    uint32_t mmfar, uint32_t bfar, uint32_t pc, uint32_t lr, uint32_t psr,
                    const char *task, const char *file, uint32_t line)
{
    /* Invalidate first so a reset in the middle of the copy is not mistaken
     * for a complete record. */
    g_app_fault.magic = 0U;
    __DMB();

    g_app_fault.reason  = (uint32_t)reason;
    g_app_fault.cfsr    = cfsr;
    g_app_fault.hfsr    = hfsr;
    g_app_fault.mmfar   = mmfar;
    g_app_fault.bfar    = bfar;
    g_app_fault.pc      = pc;
    g_app_fault.lr      = lr;
    g_app_fault.psr     = psr;
    g_app_fault.line    = line;
    g_app_fault.boot_id = app_boot_id;
    fault_copy(g_app_fault.task, sizeof(g_app_fault.task), task);
    fault_copy(g_app_fault.file, sizeof(g_app_fault.file), file);

    __DMB();
    g_app_fault.magic = APP_FAULT_MAGIC;
    __DSB();
}

void app_fault_clear(void)
{
    g_app_fault.magic = 0U;
    __DMB();
}

/**
 * @brief Busy-wait about @p ms milliseconds without interrupts or the HAL.
 * @details Uses the DWT cycle counter and SystemCoreClock; falls back to a
 *          fixed loop when the trace unit is locked.
 * @param[in] ms Delay in ms.
 */
static void fault_delay_ms(uint32_t ms)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    /* A locked trace unit never advances CYCCNT; detect it once instead of
     * spinning until the IWDG fires. */
    uint32_t probe = DWT->CYCCNT;
    for(volatile uint32_t i = 0U; i < 100U; i++)
    {
        __NOP();
    }
    if(DWT->CYCCNT == probe)
    {
        for(volatile uint32_t i = 0U; i < (ms * 20000U); i++)
        {
            __NOP();
        }
        return;
    }

    uint32_t cycles = (SystemCoreClock / 1000U) * ms;
    uint32_t start  = DWT->CYCCNT;
    while((DWT->CYCCNT - start) < cycles)
    {
        __NOP();
    }
}

/**
 * @brief Drive LD3 through BSRR.
 * @param[in] on true for on.
 */
static void fault_led3(bool on)
{
    GPIOB->BSRR = on ? APP_LED3_PIN : ((uint32_t)APP_LED3_PIN << 16U);
}

void app_fault_show_and_reset(void)
{
    __disable_irq();

    /* Register-level PB14 output set-up: DGEN-004 must work from any
     * context. */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    __DSB();
    uint32_t moder = GPIOB->MODER;
    moder &= ~(3UL << (14U * 2U));
    moder |= (1UL << (14U * 2U));
    GPIOB->MODER = moder;
    __DSB();

    /* Three short flashes and a pause, twice: about 2 s. */
    for(uint32_t cycle = 0U; cycle < 2U; cycle++)
    {
        for(uint32_t flash = 0U; flash < 3U; flash++)
        {
            fault_led3(true);
            fault_delay_ms(100U);
            fault_led3(false);
            fault_delay_ms(100U);
        }
        fault_delay_ms(500U);
    }

    NVIC_SystemReset();
    for(;;)
    {
        __NOP();
    }
}

void app_fault_fatal(app_fault_reason_t reason, const char *task, const char *file,
                     uint32_t line)
{
    app_fault_save(reason, 0U, 0U, 0U, 0U, 0U, 0U, 0U, task, file, line);
    app_fault_show_and_reset();
}

/**
 * @brief Save a CPU fault and reset; target of the naked fault handlers.
 * @param[in] frame  Stacked exception frame (MSP or PSP), may be invalid.
 * @param[in] reason One of @ref app_fault_reason_t.
 * @note Exception context. Not static: reached by name from inline asm.
 */
__attribute__((used, noreturn)) void app_fault_handler_c(uint32_t *frame, uint32_t reason)
{
    /* After a stack overflow MSP/PSP may be outside RAM, and reading the
     * frame would lock up. Read PC/LR/xPSR only when the whole frame lies in
     * internal RAM. */
    uint32_t  pc = 0U, lr = 0U, psr = 0U;
    uintptr_t base = (uintptr_t)frame;
    if(base >= 0x20000000UL && base <= 0x20080000UL && (base & 3U) == 0U &&
       (base + (8U * 4U)) <= 0x20080000UL)
    {
        pc  = frame[6];
        lr  = frame[5];
        psr = frame[7];
    }
    app_fault_save((app_fault_reason_t)reason, SCB->CFSR, SCB->HFSR, SCB->MMFAR,
                   SCB->BFAR, pc, lr, psr, NULL, NULL, 0UL);
    app_fault_show_and_reset();
}

/**
 * @brief Define a naked fault handler that passes the active stack frame and
 *        @p reason to app_fault_handler_c().
 * @param name   Vector handler name.
 * @param reason @ref app_fault_reason_t constant (immediate).
 */
#define APP_DEFINE_FAULT_HANDLER(name, reason)                                           \
    __attribute__((naked, noreturn)) void name(void)                                     \
    {                                                                                    \
        __asm volatile("tst lr, #4\n\t"                                                  \
                       "ite eq\n\t"                                                      \
                       "mrseq r0, msp\n\t"                                               \
                       "mrsne r0, psp\n\t"                                               \
                       "movs r1, %0\n\t"                                                 \
                       "b app_fault_handler_c\n\t"                                       \
                       :                                                                 \
                       : "I"(reason)                                                     \
                       : "r0", "r1", "memory");                                          \
    }

APP_DEFINE_FAULT_HANDLER(HardFault_Handler, APP_FAULT_HARDFAULT)
APP_DEFINE_FAULT_HANDLER(MemManage_Handler, APP_FAULT_MEMMANAGE)
APP_DEFINE_FAULT_HANDLER(BusFault_Handler, APP_FAULT_BUSFAULT)
APP_DEFINE_FAULT_HANDLER(UsageFault_Handler, APP_FAULT_USAGEFAULT)
