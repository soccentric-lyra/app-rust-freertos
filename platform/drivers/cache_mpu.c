/**
 * @file    cache_mpu.c
 * @brief   I/D caches and the non-cacheable DMA window.
 * @details Every DMA buffer lives in .dma_buffer in SRAM2. One MPU region makes
 *          that 16 KB normal, non-cacheable memory, so CPU and DMA agree without
 *          cache maintenance in the ISR path. The rest of the map keeps the
 *          default attributes.
 * @author  Sandesh Ghimire <sandesh@soccentric.com>
 * @date    2026-11-03
 * @target  STM32F767ZIT6
 * @srs     DGEN-003 (caches on), COM-002 (DMA buffers)
 */

#include "stm32f7xx_hal.h"
#include "cache_mpu.h"

#define DMA_MPU_REGION    6U           /**< MPU region number for .dma_buffer. */
#define DMA_MPU_BASE      0x2007C000UL /**< SRAM2 base, 16 KB. */
#define DMA_MPU_SIZE_LOG2 13U          /**< RASR.SIZE: 2^(13+1) = 16 KB. */

void app_cache_mpu_init(void)
{
    MPU->CTRL = 0U;
    __DSB();
    __ISB();

    MPU->RNR = DMA_MPU_REGION;
    MPU->RBAR =
        DMA_MPU_BASE | (DMA_MPU_REGION << MPU_RBAR_REGION_Pos) | MPU_RBAR_VALID_Msk;
    /* XN=1, AP=full access, TEX=1 C=0 B=0 (normal non-cacheable), S=0. */
    MPU->RASR = MPU_RASR_XN_Msk | (3UL << MPU_RASR_AP_Pos) | (1UL << MPU_RASR_TEX_Pos) |
                (DMA_MPU_SIZE_LOG2 << MPU_RASR_SIZE_Pos) | MPU_RASR_ENABLE_Msk;

    MPU->CTRL = MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_ENABLE_Msk;
    __DSB();
    __ISB();

    SCB_EnableICache();
    SCB_EnableDCache();
}
