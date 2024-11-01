/**
 * @file    app_log.h
 * @brief   Non-blocking debug text output on USART6 (PG14, Arduino D1).
 * @details USART3 carries HWLINK only, so debug lines go to USART6, 115200 8N1,
 *          TX only. BOOT and every application share the port and line format:
 *
 *              [   5012] R-FRT I health: passed, CONFIRMED written
 *
 * @author  Sandesh Ghimire <sandesh@soccentric.com>
 * @date    2026-11-06
 * @target  STM32F767ZIT6
 * @srs     DLOG-001..DLOG-004
 */

#ifndef APP_LOG_H
#define APP_LOG_H

#include <stdint.h>

/**
 * @defgroup app_log Debug log
 * @brief Ring-buffered, interrupt-drained debug lines on USART6.
 * @{
 */

/** @brief Configure PG14 as USART6_TX and enable interrupt-driven output. */
void app_log_init(void);

/**
 * @brief Queue one formatted line.
 * @details Formats into a stack buffer, copies it into the ring and returns; a
 *          full ring drops the line and counts it (DLOG-003). No-op before
 *          app_log_init().
 *
 * @param[in] level 'E', 'W', 'I' or 'D'.
 * @param[in] fmt   printf-style format without a trailing newline; supports
 *                  %%c %%s %%d %%i %%u %%x, width, .N for %%s and l.
 * @note Never blocks; safe from tasks, ISRs and before the scheduler.
 */
void app_log(char level, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

/**
 * @brief Drain the ring by polling with interrupts off.
 * @details Use before a deliberate reset so the last lines reach the wire.
 * @param[in] timeout_ms Upper bound in ms (loop-count estimate at 216 MHz).
 */
void app_log_flush(uint32_t timeout_ms);

/**
 * @brief Get the number of lines dropped on a full ring (DLOG-003).
 * @return Dropped-line count since boot.
 */
uint32_t app_log_dropped(void);

#define LOG_E(...) app_log('E', __VA_ARGS__) /**< Log an error line. */
#define LOG_W(...) app_log('W', __VA_ARGS__) /**< Log a warning line. */
#define LOG_I(...) app_log('I', __VA_ARGS__) /**< Log an info line. */
#define LOG_D(...) app_log('D', __VA_ARGS__) /**< Log a debug line. */

/** @} */

#endif /* APP_LOG_H */
