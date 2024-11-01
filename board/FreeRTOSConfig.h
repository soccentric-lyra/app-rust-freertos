/**
 * @file    FreeRTOSConfig.h
 * @brief   Project-owned FreeRTOS configuration.
 * @author  Sandesh Ghimire <sandesh@soccentric.com>
 * @date    2026-11-03
 * @srs     DRT-001..DRT-004; FreeRTOS V10.2.0 as shipped in the image
 *          (same kernel and port as the validation firmware)
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>

/* --- scheduler ----------------------------------------------------------- */
#define configUSE_PREEMPTION                    1 /**< Preemptive scheduling. */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1 /**< CLZ-based task selection. */
#define configUSE_TICKLESS_IDLE                 0 /**< Tick runs in idle. */
/** SYSCLK (DGEN-003). */
#define configCPU_CLOCK_HZ 216000000UL
/** 1 ms tick (DRT-001). */
#define configTICK_RATE_HZ ((TickType_t)1000)
/** Priorities 0..7. */
#define configMAX_PRIORITIES 8
/** Idle stack, words. */
#define configMINIMAL_STACK_SIZE ((uint16_t)256)
#define configMAX_TASK_NAME_LEN  16 /**< Name length incl. NUL. */
#define configUSE_16_BIT_TICKS   0  /**< 32-bit tick count. */
#define configIDLE_SHOULD_YIELD  1  /**< Idle yields to peers. */
#define configUSE_TIME_SLICING   1  /**< Round-robin equal priorities. */

/* --- memory (DRT-001, DRT-004) ------------------------------------------- */
#define configSUPPORT_STATIC_ALLOCATION  0 /**< No static kernel objects. */
#define configSUPPORT_DYNAMIC_ALLOCATION 1 /**< heap_4 allocation. */
/** Heap bytes, set by CMake. */
#define configTOTAL_HEAP_SIZE            APP_RTOS_HEAP_SIZE
#define configAPPLICATION_ALLOCATED_HEAP 0 /**< Kernel owns ucHeap. */

/* --- kernel objects ------------------------------------------------------ */
#define configUSE_MUTEXES             1 /**< Mutexes available. */
#define configUSE_RECURSIVE_MUTEXES   0 /**< No recursive mutexes. */
#define configUSE_COUNTING_SEMAPHORES 0 /**< No counting semaphores. */
#define configUSE_QUEUE_SETS          0 /**< No queue sets. */
#define configQUEUE_REGISTRY_SIZE     0 /**< No queue registry. */
#define configUSE_TASK_NOTIFICATIONS  1 /**< Task notifications (DRT-011). */

/* --- timers -------------------------------------------------------------- */
#define configUSE_TIMERS             1   /**< Software timer task enabled. */
#define configTIMER_TASK_PRIORITY    2   /**< Timer task priority. */
#define configTIMER_QUEUE_LENGTH     8   /**< Timer command queue depth. */
#define configTIMER_TASK_STACK_DEPTH 256 /**< Timer task stack, words. */

/* --- hooks (DRT-002) ----------------------------------------------------- */
#define configUSE_MALLOC_FAILED_HOOK   1 /**< vApplicationMallocFailedHook(). */
#define configCHECK_FOR_STACK_OVERFLOW 2 /**< Watermark + pointer check. */
#define configUSE_IDLE_HOOK            0 /**< No idle hook. */
#define configUSE_TICK_HOOK            0 /**< No tick hook. */

/* --- stack statistics (DRT-013) ------------------------------------------ */
#define configUSE_TRACE_FACILITY        1        /**< Task stats for Status. */
#define configRECORD_STACK_HIGH_ADDRESS 1        /**< Keep stack top address. */
#define configSTACK_DEPTH_TYPE          uint16_t /**< Stack depth type. */

/* --- interrupt priorities (DRT-003) -------------------------------------- */
/* ISRs that call FromISR APIs use priority numbers >= 5; TIM6 time base is
 * 15. */
/** NVIC priority bits on the F767. */
#define APP_NVIC_PRIO_BITS 4U
/** Priority bits for the port. */
#define configPRIO_BITS APP_NVIC_PRIO_BITS
/** Kernel ISR priority. */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY 15
/** Highest FromISR prio. */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
/** Kernel interrupt priority, shifted for the NVIC. */
#define configKERNEL_INTERRUPT_PRIORITY                                                  \
    (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
/** BASEPRI mask for kernel critical sections, shifted for the NVIC. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY                                             \
    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* --- debug (DRT-002) ----------------------------------------------------- */
extern void app_fault_assert(const char *file, int line);
/**
 * @brief Record an assert in the fault record and reset when @p x is 0.
 * @param x Condition expected to be true.
 */
#define configASSERT(x)                                                                  \
    do                                                                                   \
    {                                                                                    \
        if((x) == 0)                                                                     \
        {                                                                                \
            app_fault_assert(__FILE__, __LINE__);                                        \
        }                                                                                \
    } while(0)

/* --- port mapping (CM7 GCC port, DRT-001) -------------------------------- */
#define vPortSVCHandler     SVC_Handler     /**< Port SVC handler -> vector name. */
#define xPortPendSVHandler  PendSV_Handler  /**< Port PendSV handler -> vector. */
#define xPortSysTickHandler SysTick_Handler /**< Port tick handler -> vector. */

/* --- optional API includes ----------------------------------------------- */
#define INCLUDE_vTaskPrioritySet            0 /**< Excluded. */
#define INCLUDE_uxTaskPriorityGet           0 /**< Excluded. */
#define INCLUDE_vTaskDelete                 0 /**< Excluded; tasks never exit. */
#define INCLUDE_vTaskSuspend                1 /**< Included; portMAX_DELAY waits. */
#define INCLUDE_vTaskDelayUntil             1 /**< Included. */
#define INCLUDE_vTaskDelay                  1 /**< Included. */
#define INCLUDE_xTaskGetSchedulerState      1 /**< Included; platform.c uses it. */
#define INCLUDE_xTaskGetCurrentTaskHandle   1 /**< Included; notification binds. */
#define INCLUDE_uxTaskGetStackHighWaterMark 1 /**< Included; Status stack stats. */

#endif /* FREERTOS_CONFIG_H */
