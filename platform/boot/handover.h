/**
 * @file    handover.h
 * @brief   BOOT hand-over record (RTC_BKP2R) and "stay in BOOT" request.
 * @author  Sandesh Ghimire <sandesh@soccentric.com>
 * @date    2026-11-03
 * @target  STM32F767ZIT6
 * @srs     DBT-004, DBT-005
 */

#ifndef HANDOVER_H
#define HANDOVER_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @defgroup handover BOOT hand-over
 * @brief Decoded RTC_BKP2R record and the RTC_BKP1R request.
 * @{
 */

/**
 * @brief Hand-over record decoded from RTC_BKP2R.
 */
typedef struct
{
    bool     present;     /**< Magic and slot are valid. */
    uint8_t  slot;        /**< APP_SLOT_*. */
    uint8_t  trial;       /**< 0 unknown, 1 confirmed, 2 pending. */
    uint8_t  trial_count; /**< BOOT trial counter, 0..3. */
    uint32_t raw;         /**< Raw RTC_BKP2R word. */
} app_handover_t;

/** Record decoded by app_handover_read(); read-only afterwards. */
extern app_handover_t g_handover;

/**
 * @brief Decode RTC_BKP2R into @ref g_handover.
 * @details Reads only; backup-domain write access stays off. A missing or
 *          invalid record gives slot UNKNOWN and trial 0 (DBT-004).
 */
void app_handover_read(void);

/**
 * @brief Enable backup-domain access and write the "stay in BOOT" magic.
 * @details Writes @ref APP_STAY_IN_BOOT_MAGIC to RTC_BKP1R (DBT-005). The caller
 *          resets afterwards.
 */
void app_handover_request_bootloader(void);

/** @} */

#endif /* HANDOVER_H */
