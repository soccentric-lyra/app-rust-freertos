/**
 * @file    stm32f7xx_hal_conf.h
 * @brief   Project HAL configuration.
 * @author  Sandesh Ghimire <sandesh@soccentric.com>
 * @date    2026-11-03
 * @target  STM32F767ZIT6, NUCLEO-F767ZI
 * @srs     DCON-002 (drivers used as shipped; this is the project-owned
 *          configuration file)
 */

#ifndef STM32F7xx_HAL_CONF_H
#define STM32F7xx_HAL_CONF_H

/* Oscillators in Hz. PH0 carries the ST-LINK 8 MHz MCO in bypass mode
 * (Hardware.md section 6). */
#define HSE_VALUE                 8000000U  /**< HSE frequency. */
#define HSE_STARTUP_TIMEOUT       100U      /**< HSE start-up timeout, ms. */
#define HSI_VALUE                 16000000U /**< HSI frequency. */
#define LSI_VALUE                 32000U    /**< LSI nominal frequency. */
#define LSE_VALUE                 32768U    /**< LSE frequency. */
#define LSE_STARTUP_TIMEOUT       5000U     /**< LSE start-up timeout, ms. */
#define EXTERNAL_SAI1_CLOCK_VALUE 48000U    /**< SAI1 external clock (unused). */
#define EXTERNAL_SAI2_CLOCK_VALUE 48000U    /**< SAI2 external clock (unused). */
#define EXTERNAL_CLOCK_VALUE      12288000U /**< I2S external clock (unused). */

#define VDD_VALUE         3300U /**< VDD in mV. */
#define TICK_INT_PRIORITY 15U   /**< TIM6 time base, below the kernel (DRT-003). */
#define USE_RTOS          0U    /**< HAL locking without RTOS support. */
#define PREFETCH_ENABLE   1U    /**< Flash prefetch on. */

/* Enabled HAL modules; the list grows with each SRS section. */
#define HAL_MODULE_ENABLED        /**< HAL core. */
#define HAL_CORTEX_MODULE_ENABLED /**< NVIC/SysTick/MPU helpers. */
#define HAL_RCC_MODULE_ENABLED    /**< Clock control. */
#define HAL_GPIO_MODULE_ENABLED   /**< GPIO. */
#define HAL_PWR_MODULE_ENABLED    /**< Power control, backup access. */
#define HAL_FLASH_MODULE_ENABLED  /**< RCC latency macros; HLT-002. */
#define HAL_DMA_MODULE_ENABLED    /**< USART3 RX/TX DMA (COM-002). */
#define HAL_UART_MODULE_ENABLED   /**< HWLINK on USART3 (COM-001). */
#define HAL_IWDG_MODULE_ENABLED   /**< Health supervision (DRT-012). */

#include "stm32f7xx_hal_rcc.h"
#include "stm32f7xx_hal_gpio.h"
#include "stm32f7xx_hal_cortex.h"
#include "stm32f7xx_hal_pwr.h"
#include "stm32f7xx_hal_pwr_ex.h"
#include "stm32f7xx_hal_flash.h"
#include "stm32f7xx_hal_flash_ex.h"
#include "stm32f7xx_hal_dma.h"
#include "stm32f7xx_hal_uart.h"
#include "stm32f7xx_hal_iwdg.h"

#ifdef USE_FULL_ASSERT
/**
 * @brief Report a failed HAL parameter check.
 * @param[in] file Source file.
 * @param[in] line Source line.
 */
void assert_failed(uint8_t *file, uint32_t line);
#endif

#ifdef USE_FULL_ASSERT
/**
 * @brief HAL parameter check; compiled out unless USE_FULL_ASSERT.
 * @param expr Condition expected to be true.
 */
#define assert_param(expr)                                                               \
    ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
#else
#define assert_param(expr) ((void)0U) /**< Parameter checks compiled out. */
#endif

#endif /* STM32F7xx_HAL_CONF_H */
