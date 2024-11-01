/**
 * @file    flash_confirm.h
 * @brief   Program the CONFIRMED metadata record of the running slot.
 * @author  Sandesh Ghimire <sandesh@soccentric.com>
 * @date    2026-11-03
 * @target  STM32F767ZIT6, dual-bank flash
 * @srs     HLT-002, HLT-005, DBT-006, DSAFE-002
 */

#ifndef FLASH_CONFIRM_H
#define FLASH_CONFIRM_H

#include <stdbool.h>

/**
 * @defgroup flash_confirm CONFIRMED record writer
 * @brief The application's only flash write.
 * @{
 */

/**
 * @brief Program the running slot's CONFIRMED record if it is erased.
 * @details The address is fixed by @ref APP_CONFIRMED_ADDR; no erase is issued.
 *          Blocks for the flash program time. A torn record is left alone.
 *
 * @return true when a valid record is present after the call.
 * @retval false Record torn/foreign, unlock or program failed, or verify failed.
 */
bool app_flash_confirm_running_slot(void);

/** @} */

#endif /* FLASH_CONFIRM_H */
