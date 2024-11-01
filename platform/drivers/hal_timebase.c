/**
 * @file    hal_timebase.c
 * @brief   HAL 1 ms tick on TIM6 (DGEN-006).
 * @details SysTick belongs to FreeRTOS (DRT-001), so the HAL tick uses TIM6.
 *          Same scheme as the validation firmware (HWVAL GEN-012).
 * @author  Sandesh Ghimire <sandesh@soccentric.com>
 * @date    2026-11-03
 * @target  STM32F767ZIT6
 * @srs     DGEN-006
 */

#include "stm32f7xx_hal.h"

void TIM6_DAC_IRQHandler(void);

/**
 * @brief Start TIM6 as a 1 kHz update interrupt for the HAL tick.
 * @details Overrides the weak HAL version; called by HAL_Init() and
 *          HAL_RCC_ClockConfig().
 * @param[in] TickPriority NVIC priority for TIM6_DAC_IRQn.
 * @return HAL_OK always.
 */
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
    /* APB timers run at 2 x PCLK unless the APB prescaler is 1 (RM0410). */
    uint32_t timclk    = ((RCC->CFGR & RCC_CFGR_PPRE1) == 0U) ? pclk1 : pclk1 * 2U;
    uint32_t prescaler = (timclk / 1000000U) - 1U;

    __HAL_RCC_TIM6_CLK_ENABLE();

    TIM6->CR1  = 0U;
    TIM6->PSC  = prescaler;  /* 1 MHz count */
    TIM6->ARR  = 1000U - 1U; /* 1 ms period */
    TIM6->EGR  = TIM_EGR_UG; /* latch PSC/ARR, reset counter */
    TIM6->SR   = 0U;
    TIM6->DIER = TIM_DIER_UIE;
    TIM6->CR1  = TIM_CR1_CEN | TIM_CR1_URS;

    uwTickPrio = TickPriority;
    NVIC_SetPriority(TIM6_DAC_IRQn, TickPriority);
    NVIC_EnableIRQ(TIM6_DAC_IRQn);

    return HAL_OK;
}

/** @brief Stop the HAL tick by masking the TIM6 update interrupt. */
void HAL_SuspendTick(void) { TIM6->DIER &= ~TIM_DIER_UIE; }

/** @brief Resume the HAL tick; clears a pending update first. */
void HAL_ResumeTick(void)
{
    TIM6->SR = 0U;
    TIM6->DIER |= TIM_DIER_UIE;
}

/** @brief TIM6 update ISR: advance uwTick by uwTickFreq. */
void TIM6_DAC_IRQHandler(void)
{
    if((TIM6->SR & TIM_SR_UIF) != 0U)
    {
        TIM6->SR = ~TIM_SR_UIF;
        uwTick += uwTickFreq;
    }
}
