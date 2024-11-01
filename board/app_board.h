/**
 * @file    app_board.h
 * @brief   NUCLEO-F767ZI pins and APB prescalers used by the application.
 * @author  Sandesh Ghimire <sandesh@soccentric.com>
 * @date    2026-11-03
 * @target  NUCLEO-F767ZI (STM32F767ZIT6)
 * @srs     DASM-002, DSAFE-003; pins from Hardware.md section 9
 */

#ifndef APP_BOARD_H
#define APP_BOARD_H

#include "stm32f7xx_hal.h"

/**
 * @defgroup app_board Board pins
 * @brief LED, button and clock-tree constants for NUCLEO-F767ZI.
 * @{
 */

#define APP_LED1_PORT GPIOB       /**< LD1 green port; high = on. */
#define APP_LED1_PIN  GPIO_PIN_0  /**< LD1 green pin (PB0). */
#define APP_LED2_PORT GPIOB       /**< LD2 blue port; high = on. */
#define APP_LED2_PIN  GPIO_PIN_7  /**< LD2 blue pin (PB7). */
#define APP_LED3_PORT GPIOB       /**< LD3 red port; errors only (LED-005). */
#define APP_LED3_PIN  GPIO_PIN_14 /**< LD3 red pin (PB14). */
#define APP_BTN1_PORT GPIOC       /**< B1 user button port; high when pressed. */
#define APP_BTN1_PIN  GPIO_PIN_13 /**< B1 user button pin (PC13). */

/** APB1 prescaler: 54 MHz, timers 108 MHz (DGEN-003). */
#define APP_APB1_PRESCALER RCC_HCLK_DIV4
/** APB2 prescaler: 108 MHz, timers 216 MHz (DGEN-003). */
#define APP_APB2_PRESCALER RCC_HCLK_DIV2

/** @} */

#endif /* APP_BOARD_H */
