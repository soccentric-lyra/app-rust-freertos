/**
 * @file    iwdg.h
 * @brief   Independent watchdog start and refresh.
 * @author  Sandesh Ghimire <sandesh@soccentric.com>
 * @date    2026-11-03
 * @target  STM32F767ZIT6
 * @srs     DBT-002, DRT-012, HLT-003, HND-004
 */

#ifndef IWDG_H
#define IWDG_H

/**
 * @defgroup iwdg Independent watchdog
 * @brief 4 s IWDG shared with BOOT.
 * @{
 */

/**
 * @brief Refresh the IWDG at the first instruction of main().
 * @details BOOT starts the IWDG with a 4 s timeout before hand-over (HND-004).
 *          Safe before the HAL and clocks are up.
 */
void app_iwdg_early_kick(void);

/** @brief Start the IWDG at 4 s unless BOOT already started it, then refresh. */
void app_iwdg_init(void);

/**
 * @brief Refresh the IWDG.
 * @note The Health task calls this only while every task checks in.
 */
void app_iwdg_kick(void);

/** @} */

#endif /* IWDG_H */
