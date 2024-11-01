/**
 * @file    reset_cause.h
 * @brief   RCC reset flags captured at start-up.
 * @author  Sandesh Ghimire <sandesh@soccentric.com>
 * @date    2026-11-03
 * @target  STM32F767ZIT6
 * @srs     DGEN-005, LED-005
 */

#ifndef RESET_CAUSE_H
#define RESET_CAUSE_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @defgroup reset_cause Reset cause
 * @brief Saved RCC_CSR reset flags and their text.
 * @{
 */

/** RCC_CSR reset flags (RCC_CSR_*RSTF) saved by app_reset_cause_capture(). */
extern uint32_t app_reset_cause_flags;

/** Most specific reset cause as text; "unknown" before capture. */
extern const char *app_reset_cause_text;

/** @brief Save RCC_CSR reset flags for Status, then clear them in RCC. */
void app_reset_cause_capture(void);

/**
 * @brief Check whether the last reset came from a watchdog.
 * @return true for an IWDG or WWDG reset; LD3 latches on (LED-005).
 */
bool app_reset_was_error(void);

/** @} */

#endif /* RESET_CAUSE_H */
