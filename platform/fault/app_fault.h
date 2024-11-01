/**
 * @file    app_fault.h
 * @brief   No-init fault record and fatal reset paths.
 * @author  Sandesh Ghimire <sandesh@soccentric.com>
 * @date    2026-11-03
 * @target  STM32F767ZIT6
 * @srs     DGEN-004, DRT-002, LED-005
 */

#ifndef APP_FAULT_H
#define APP_FAULT_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @defgroup app_fault Fault record
 * @brief Persistent fault record, LD3 fault pattern and reset.
 * @{
 */

#define APP_FAULT_MAGIC    0x4641554CUL /**< "FAUL": record is valid. */
#define APP_FAULT_TASK_LEN 16U          /**< Task name field, incl. NUL. */
#define APP_FAULT_FILE_LEN 32U          /**< File basename field, incl. NUL. */

/**
 * @brief Reason stored in the fault record.
 * @note Values are reported by name in Status; do not renumber.
 */
typedef enum
{
    APP_FAULT_NONE = 0,       /**< No fault. */
    APP_FAULT_HARDFAULT,      /**< HardFault exception. */
    APP_FAULT_MEMMANAGE,      /**< MemManage exception. */
    APP_FAULT_BUSFAULT,       /**< BusFault exception. */
    APP_FAULT_USAGEFAULT,     /**< UsageFault exception. */
    APP_FAULT_ASSERT,         /**< configASSERT or Rust panic. */
    APP_FAULT_STACK_OVERFLOW, /**< FreeRTOS stack overflow hook. */
    APP_FAULT_MALLOC_FAILED,  /**< FreeRTOS malloc-failed hook. */
    APP_FAULT_STARTUP         /**< Start-up step failed. */
} app_fault_reason_t;

/**
 * @brief Fault record kept in .noinit across resets.
 * @note Layout must fit @ref APP_NOINIT_SIZE with the boot id (static assert).
 */
typedef struct
{
    uint32_t magic;                    /**< @ref APP_FAULT_MAGIC when valid. */
    uint32_t reason;                   /**< One of @ref app_fault_reason_t. */
    uint32_t cfsr;                     /**< SCB->CFSR at the fault, else 0. */
    uint32_t hfsr;                     /**< SCB->HFSR at the fault, else 0. */
    uint32_t mmfar;                    /**< SCB->MMFAR at the fault, else 0. */
    uint32_t bfar;                     /**< SCB->BFAR at the fault, else 0. */
    uint32_t pc;                       /**< Stacked PC, 0 if frame unusable. */
    uint32_t lr;                       /**< Stacked LR, 0 if frame unusable. */
    uint32_t psr;                      /**< Stacked xPSR, 0 if frame unusable. */
    uint32_t line;                     /**< Source line, or hook-specific value. */
    uint32_t boot_id;                  /**< Boot counter when saved. */
    char     task[APP_FAULT_TASK_LEN]; /**< NUL-terminated task name. */
    char     file[APP_FAULT_FILE_LEN]; /**< NUL-terminated file basename. */
} app_fault_record_t;

/** Fault record in .noinit; start-up code leaves it alone (DGEN-004). */
extern app_fault_record_t g_app_fault;

/** @brief Increment the no-init boot id; call at the first instructions of main. */
void app_fault_boot_start(void);

/**
 * @brief Check the record magic and reason range.
 * @return true when the record is valid.
 */
bool app_fault_is_valid(void);

/**
 * @brief Check for a valid record saved during this boot.
 * @return true when valid and saved with the current boot id.
 */
bool app_fault_is_current_boot(void);

/**
 * @brief Check for a valid record saved during the previous boot.
 * @return true when valid and saved with the previous boot id.
 */
bool app_fault_is_previous_boot(void);

/**
 * @brief Map a reason to its Status name.
 * @param[in] reason Fault reason.
 * @return Static string; "unknown" when out of range.
 */
const char *app_fault_reason_text(app_fault_reason_t reason);

/**
 * @brief Get the record's task name.
 * @return NUL-terminated task field of @ref g_app_fault.
 */
const char *app_fault_task(void);

/**
 * @brief Get the record's file name.
 * @return NUL-terminated file field of @ref g_app_fault.
 */
const char *app_fault_file(void);

/**
 * @brief Write the fault record.
 * @details Clears the magic first and sets it last, so a reset mid-write never
 *          leaves a valid-looking partial record.
 *
 * @param[in] reason Fault reason.
 * @param[in] cfsr   SCB->CFSR.
 * @param[in] hfsr   SCB->HFSR.
 * @param[in] mmfar  SCB->MMFAR.
 * @param[in] bfar   SCB->BFAR.
 * @param[in] pc     Stacked PC.
 * @param[in] lr     Stacked LR.
 * @param[in] psr    Stacked xPSR.
 * @param[in] task   Task name or NULL; truncated to 15 chars.
 * @param[in] file   File path or NULL; basename kept, truncated to 31 chars.
 * @param[in] line   Source line or hook value.
 * @note Safe from fault handlers and with interrupts disabled.
 */
void app_fault_save(app_fault_reason_t reason, uint32_t cfsr, uint32_t hfsr,
                    uint32_t mmfar, uint32_t bfar, uint32_t pc, uint32_t lr, uint32_t psr,
                    const char *task, const char *file, uint32_t line);

/**
 * @brief Disable IRQs, flash LD3 for about 2 s and reset.
 * @note Register-level only; works from any context, before the HAL.
 */
void app_fault_show_and_reset(void) __attribute__((noreturn));

/**
 * @brief Record a fatal software error, show it on LD3 and reset.
 * @param[in] reason Fault reason.
 * @param[in] task   Task name or NULL.
 * @param[in] file   File or location text, or NULL.
 * @param[in] line   Line or hook value.
 */
void app_fault_fatal(app_fault_reason_t reason, const char *task, const char *file,
                     uint32_t line) __attribute__((noreturn));

/** @brief Invalidate the fault record. */
void app_fault_clear(void);

/** @} */

#endif /* APP_FAULT_H */
