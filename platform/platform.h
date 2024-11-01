/**
 * @file    platform.h
 * @brief   C platform layer called from the Rust application.
 * @details Drivers and the BOOT contract stay in C (HAL, FreeRTOS); no
 *          application logic lives here. src/platform/plat.rs declares the
 *          same functions; keep both files in step.
 * @author  Sandesh Ghimire <sandesh@soccentric.com>
 * @srs     RFCON-004, RFBT-002, RFBT-005, RFRT-004, RFGEN-004
 */

#ifndef RFDEMO_PLATFORM_H
#define RFDEMO_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @defgroup platform Rust platform API
 * @brief extern "C" functions behind src/platform/plat.rs.
 * @{
 */

#define PLAT_FAULT_TASK_LEN 16U /**< Task field length, = APP_FAULT_TASK_LEN. */
#define PLAT_FAULT_FILE_LEN 32U /**< File field length, = APP_FAULT_FILE_LEN. */

/**
 * @brief Decoded copy of the no-init fault record.
 * @note ABI shared with the Rust `Fault` struct (`#[repr(C)]`); do not reorder.
 */
typedef struct
{
    uint32_t reason;                    /**< app_fault_reason_t value. */
    uint32_t cfsr;                      /**< SCB->CFSR. */
    uint32_t hfsr;                      /**< SCB->HFSR. */
    uint32_t mmfar;                     /**< SCB->MMFAR. */
    uint32_t bfar;                      /**< SCB->BFAR. */
    uint32_t pc;                        /**< Stacked PC. */
    uint32_t lr;                        /**< Stacked LR. */
    uint32_t psr;                       /**< Stacked xPSR. */
    uint32_t line;                      /**< Line or hook value. */
    char     task[PLAT_FAULT_TASK_LEN]; /**< NUL-terminated task name. */
    char     file[PLAT_FAULT_FILE_LEN]; /**< NUL-terminated file basename. */
} plat_fault_t;

/** Longest text plat_log() copies, in bytes (= Rust log TEXT_MAX). */
#define PLAT_LOG_TEXT_MAX 120U

/**
 * @brief Log one debug line on USART6 (RFDBG-001).
 * @param[in] level 'E', 'W', 'I' or 'D'.
 * @param[in] text  Text, not NUL-terminated; NULL logs nothing.
 * @param[in] len   Length; truncated to @ref PLAT_LOG_TEXT_MAX.
 */
void plat_log(uint8_t level, const char *text, uint32_t len);

/** @brief Increment the no-init boot id (RFGEN-004). */
void plat_fault_boot_start(void);

/**
 * @brief Copy the fault record without magic and boot id.
 * @param[out] out Destination.
 * @return true when a valid record was copied.
 */
bool plat_fault_get(plat_fault_t *out);

/**
 * @brief Check for a fault saved in this boot.
 * @return true when valid and from the current boot.
 */
bool plat_fault_is_current_boot(void);

/**
 * @brief Check for a fault saved in the previous boot.
 * @return true when valid and from the previous boot.
 */
bool plat_fault_is_previous_boot(void);

/** @brief Invalidate the fault record. */
void plat_fault_clear(void);

/** @brief Record APP_FAULT_ASSERT ("rust", "panic"), flash LD3 and reset. */
void plat_fatal(void) __attribute__((noreturn));

/**
 * @brief Read SCB->VTOR.
 * @return Vector table address.
 */
uint32_t plat_vtor(void);

/** @brief Flush the debug log and reset. */
void plat_reboot(void) __attribute__((noreturn));

/**
 * @brief Invalidate D-cache lines covering a range.
 * @param[in] addr Start address; 0 is a no-op.
 * @param[in] len  Length in bytes; 0 is a no-op.
 */
void plat_dcache_invalidate(uintptr_t addr, uint32_t len);

/**
 * @brief Name the calling thread; no-op (names are fixed in main.c).
 * @param[in] name Ignored.
 */
void plat_thread_name(const char *name);

/**
 * @brief Get the calling task's unused stack (RFRT-005).
 * @return High-water mark in bytes; 0 before the scheduler starts.
 */
uint32_t plat_stack_unused(void);

/**
 * @brief Get the reset flags captured at start-up.
 * @return RCC_CSR *RSTF bits.
 */
uint32_t plat_reset_cause_flags(void);

/**
 * @brief Get the hand-over slot.
 * @return APP_SLOT_*.
 */
uint8_t plat_handover_slot(void);

/**
 * @brief Get the hand-over trial state.
 * @return 0 unknown, 1 confirmed, 2 pending.
 */
uint8_t plat_handover_trial(void);

/**
 * @brief Get the BOOT trial counter.
 * @return 0..3.
 */
uint8_t plat_handover_trial_count(void);

/** @brief Request "stay in BOOT" (RFBT-005), flush the log and reset. */
void plat_enter_bootloader(void) __attribute__((noreturn));

/**
 * @brief Get the time since boot.
 * @return Milliseconds; HAL tick before the scheduler, RTOS tick after.
 */
uint32_t plat_now_ms(void);

/**
 * @brief Sleep the calling task; busy-waits before the scheduler starts.
 * @param[in] ms Duration in ms; at least one tick once the scheduler runs.
 */
void plat_sleep_ms(uint32_t ms);

/**
 * @brief Start (or adopt) the IWDG (RFRT-004).
 * @return true always.
 */
bool plat_wdt_init(void);

/** @brief Refresh the IWDG. */
void plat_wdt_feed(void);

/**
 * @brief Configure LD1..LD3 and B1 with its EXTI.
 * @return true always.
 */
bool plat_gpio_init(void);

/**
 * @brief Switch one LED.
 * @param[in] led 0 = LD1, 1 = LD2, 2 = LD3; others ignored.
 * @param[in] on  true for on.
 */
void plat_led_set(uint32_t led, bool on);

/**
 * @brief Read B1.
 * @return true while pressed.
 */
bool plat_button_pressed(void);

/**
 * @brief Block until a B1 edge or the timeout.
 * @param[in] timeout_ms Timeout in ms.
 * @pre plat_button_bind() called from the same task.
 */
void plat_button_wait(uint32_t timeout_ms);

/** @brief Make the calling task the target of B1 edge notifications. */
void plat_button_bind(void);

/**
 * @brief Create the RX wake queue and start USART3 (RFN-003).
 * @return true on success.
 */
bool plat_uart_init(void);

/**
 * @brief Switch USART3 to 115200 or 921600 baud.
 * @param[in] baud Rate in bit/s.
 * @return true when active.
 */
bool plat_uart_set_baud(uint32_t baud);

/**
 * @brief Copy received bytes from the RX ring.
 * @param[out] dst     Destination.
 * @param[in]  max_len Capacity of @p dst.
 * @return Bytes copied.
 */
uint32_t plat_uart_read(uint8_t *dst, uint32_t max_len);

/**
 * @brief Block until RX activity or the timeout.
 * @param[in] timeout_ms Timeout in ms.
 */
void plat_uart_wait(uint32_t timeout_ms);

/**
 * @brief Check the RX DMA receiver.
 * @return true while armed.
 */
bool plat_uart_rx_running(void);

/**
 * @brief Send one frame and wait for the last stop bit.
 * @param[in] data       Bytes to send.
 * @param[in] len        Length; values above 65535 are truncated.
 * @param[in] timeout_ms DMA completion timeout in ms.
 * @return true when sent.
 */
bool plat_uart_send(const uint8_t *data, uint32_t len, uint32_t timeout_ms);

/** @brief No-op; kept for the platform contract (RX wakes through a queue). */
void plat_uart_bind(void);

/**
 * @brief Copy the running slot's 32-byte CONFIRMED record (RFBT-006).
 * @details The address is fixed here, never passed in.
 * @param[out] out32 32-byte destination.
 * @return false only when @p out32 is NULL.
 */
bool plat_confirmed_read(uint8_t *out32);

/**
 * @brief Program the CONFIRMED record if erased.
 * @return true when a valid record is present afterwards.
 */
bool plat_confirm_running_slot(void);

/** @} */

#endif /* RFDEMO_PLATFORM_H */
