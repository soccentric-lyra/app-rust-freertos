/**
 * @file    clock.h
 * @brief   216 MHz core clock (DGEN-003).
 * @author  Sandesh Ghimire <sandesh@soccentric.com>
 * @date    2026-11-03
 * @srs     DGEN-003, DBT-003
 */

#ifndef CLOCK_H
#define CLOCK_H

/**
 * @defgroup clock Clock tree
 * @brief System clock configuration.
 * @{
 */

/**
 * @brief Configure HSE bypass + PLL for 216 MHz SYSCLK, APB1 54 MHz, APB2 108 MHz.
 * @details Starts from the reset clock state BOOT leaves (HND-001). Calls the
 *          fatal path (no return) when the oscillator, over-drive or bus clocks
 *          fail.
 */
void app_clock_init(void);

/** @} */

#endif /* CLOCK_H */
