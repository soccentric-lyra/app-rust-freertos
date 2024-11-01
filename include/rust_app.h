/**
 * @file    rust_app.h
 * @brief   Entry points exported by the Rust staticlib.
 * @details src/lib.rs and the task modules define these with
 *          `#[no_mangle] extern "C"`; keep both sides in step.
 * @author  Sandesh Ghimire <sandesh@soccentric.com>
 * @srs     RFCON-003, RFRT-001..RFRT-005
 */

#ifndef RUST_APP_H
#define RUST_APP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @defgroup rust_app Rust application entry points
     * @brief Functions the C start-up calls into Rust.
     * @{
     */

    /**
     * @brief Decode the boot record, log start-up, check VTOR, publish status.
     * @details Runs in the C main thread before the scheduler starts, after HAL,
     *          clocks, UART, GPIO and IWDG init. Calls the fatal path when VTOR is
     *          not the slot base.
     *
     * @param[in] reset_cause RCC_CSR reset flags captured by C.
     * @param[in] slot        APP_SLOT_* from the hand-over record.
     * @param[in] trial       0 unknown, 1 confirmed, 2 pending.
     * @param[in] trial_count BOOT trial counter, 0..3.
     */
    void rust_startup(uint32_t reset_cause, uint8_t slot, uint8_t trial,
                      uint8_t trial_count);

    /**
     * @brief Health task: supervise check-ins, feed the IWDG, confirm the slot.
     * @param[in] arg Unused.
     * @note FreeRTOS TaskFunction_t; never returns.
     */
    void rust_health_task(void *arg);

    /**
     * @brief Comms task: serve HWLINK on USART3.
     * @param[in] arg Unused.
     * @note FreeRTOS TaskFunction_t; never returns.
     */
    void rust_comms_task(void *arg);

    /**
     * @brief Button task: debounce B1 and send short/long press commands.
     * @param[in] arg Unused.
     * @note FreeRTOS TaskFunction_t; never returns.
     */
    void rust_button_task(void *arg);

    /**
     * @brief LED task: drive the LD1..LD3 patterns.
     * @param[in] arg Unused.
     * @note FreeRTOS TaskFunction_t; never returns.
     */
    void rust_led_task(void *arg);

    /** @} */

#ifdef __cplusplus
}
#endif

#endif /* RUST_APP_H */
