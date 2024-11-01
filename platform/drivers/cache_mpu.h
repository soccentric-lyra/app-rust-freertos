/**
 * @file    cache_mpu.h
 * @brief   I/D caches and the non-cacheable DMA window.
 * @author  Sandesh Ghimire <sandesh@soccentric.com>
 * @date    2026-11-03
 * @target  STM32F767ZIT6
 * @srs     DGEN-003 (caches on), COM-002 (DMA buffers)
 */

#ifndef CACHE_MPU_H
#define CACHE_MPU_H

/**
 * @defgroup cache_mpu Caches and MPU
 * @brief Cache enable and the .dma_buffer MPU region.
 * @{
 */

/**
 * @brief Make .dma_buffer (SRAM2) non-cacheable, then enable I- and D-cache.
 * @pre Called once, before any DMA transfer starts.
 */
void app_cache_mpu_init(void);

/** @} */

#endif /* CACHE_MPU_H */
