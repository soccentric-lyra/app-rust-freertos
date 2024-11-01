/**
 * @file    app_log.c
 * @brief   Non-blocking debug text output on USART6 (PG14, Arduino D1).
 * @details DLOG-002: a 1 KB ring drained by the USART6 TXE interrupt at the
 *          lowest priority; the ISR makes no FreeRTOS call. Writers mask
 *          PRIMASK only while copying one line. The formatter is local because
 *          newlib-nano's snprintf pulls in malloc and there is no C heap.
 * @author  Sandesh Ghimire <sandesh@soccentric.com>
 * @date    2026-11-06
 * @target  STM32F767ZIT6
 * @srs     DLOG-001..DLOG-004
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include "stm32f7xx_hal.h"
#include "app_log.h"

#define LOG_BAUD         115200U /**< USART6 baud rate. */
#define LOG_RING_SIZE    1024U   /**< Ring size in bytes; power of two. */
#define LOG_LINE_MAX     128U    /**< Longest formatted line incl. CR LF. */
#define LOG_TAG          "R-FRT" /**< Firmware tag in every line. */
#define LOG_IRQ_PRIORITY 15U     /**< Lowest NVIC priority; ISR has no RTOS calls. */

void USART6_IRQHandler(void);

static char              log_ring[LOG_RING_SIZE]; /**< TX ring buffer. */
static volatile uint32_t log_head;    /**< Free-running write count (writers). */
static volatile uint32_t log_tail;    /**< Free-running read count (ISR/flush). */
static volatile uint32_t log_dropped; /**< Lines dropped on a full ring. */
static bool              log_ready;   /**< Set once app_log_init() ran. */

void app_log_init(void)
{
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_USART6_CLK_ENABLE();

    GPIO_InitTypeDef tx = {.Pin       = GPIO_PIN_14,
                           .Mode      = GPIO_MODE_AF_PP,
                           .Pull      = GPIO_NOPULL,
                           .Speed     = GPIO_SPEED_FREQ_MEDIUM,
                           .Alternate = GPIO_AF8_USART6};
    HAL_GPIO_Init(GPIOG, &tx);

    USART6->CR1 = 0U;
    USART6->BRR = (HAL_RCC_GetPCLK2Freq() + (LOG_BAUD / 2U)) / LOG_BAUD;
    USART6->CR1 = USART_CR1_TE | USART_CR1_UE;

    HAL_NVIC_SetPriority(USART6_IRQn, LOG_IRQ_PRIORITY, 0U);
    HAL_NVIC_EnableIRQ(USART6_IRQn);
    log_ready = true;
}

/**
 * @brief Copy a whole line into the ring and enable TXE, or drop it.
 * @param[in] text Line bytes.
 * @param[in] len  Length in bytes.
 * @note Masks interrupts for the copy; safe from any context.
 */
static void log_push(const char *text, uint32_t len)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    if(len > LOG_RING_SIZE - (log_head - log_tail))
    {
        log_dropped++;
    }
    else
    {
        for(uint32_t i = 0U; i < len; i++)
        {
            log_ring[(log_head + i) & (LOG_RING_SIZE - 1U)] = text[i];
        }
        log_head += len;
        USART6->CR1 |= USART_CR1_TXEIE;
    }
    __set_PRIMASK(primask);
}

/** @brief Bounded output buffer for the formatter. */
typedef struct
{
    char  *buf; /**< Destination storage. */
    size_t len; /**< Bytes written. */
    size_t cap; /**< Bytes allowed; extra output is discarded. */
} log_out_t;

/**
 * @brief Append one character if there is room.
 * @param[in,out] out Output buffer.
 * @param[in]     c   Character.
 */
static void out_char(log_out_t *out, char c)
{
    if(out->len < out->cap)
    {
        out->buf[out->len] = c;
        out->len++;
    }
}

/**
 * @brief Append an unsigned magnitude in @p base, padded to @p width.
 * @param[in,out] out      Output buffer.
 * @param[in]     value    Magnitude.
 * @param[in]     base     10 or 16 (lower-case hex).
 * @param[in]     negative Prefix '-'.
 * @param[in]     width    Minimum field width including the sign.
 * @param[in]     pad      ' ' or '0'; padding goes before the sign.
 */
static void out_number(log_out_t *out, uint32_t value, uint32_t base, bool negative,
                       uint32_t width, char pad)
{
    char     digits[11];
    uint32_t n = 0U;
    do
    {
        uint32_t d = value % base;
        digits[n]  = (char)(d < 10U ? '0' + d : 'a' + d - 10U);
        n++;
        value /= base;
    } while(value != 0U);
    if(negative)
    {
        digits[n] = '-';
        n++;
    }
    for(uint32_t i = n; i < width; i++)
    {
        out_char(out, pad);
    }
    while(n > 0U)
    {
        n--;
        out_char(out, digits[n]);
    }
}

/**
 * @brief Minimal vprintf: %%, c, s (.N), d, i, u, x, width, 0-pad, l.
 * @param[in,out] out  Output buffer.
 * @param[in]     fmt  Format string.
 * @param[in]     args Arguments.
 * @note Unsupported conversions are printed literally.
 */
static void log_format(log_out_t *out, const char *fmt, va_list args)
{
    for(; *fmt != '\0'; fmt++)
    {
        if(*fmt != '%')
        {
            out_char(out, *fmt);
            continue;
        }
        fmt++;
        char pad = ' ';
        if(*fmt == '0')
        {
            pad = '0';
            fmt++;
        }
        uint32_t width = 0U;
        while(*fmt >= '0' && *fmt <= '9')
        {
            width = (width * 10U) + (uint32_t)(*fmt - '0');
            fmt++;
        }
        uint32_t precision = UINT32_MAX;
        if(*fmt == '.')
        {
            precision = 0U;
            fmt++;
            while(*fmt >= '0' && *fmt <= '9')
            {
                precision = (precision * 10U) + (uint32_t)(*fmt - '0');
                fmt++;
            }
        }
        if(*fmt == 'l')
        {
            fmt++; /* long and int are both 32 bits here */
        }
        switch(*fmt)
        {
        case 'c': out_char(out, (char)va_arg(args, int)); break;
        case 's':
        {
            const char *text = va_arg(args, const char *);
            for(uint32_t i = 0U; text != NULL && text[i] != '\0' && i < precision; i++)
            {
                out_char(out, text[i]);
            }
            break;
        }
        case 'd':
        case 'i':
        {
            int32_t value = va_arg(args, int32_t);
            out_number(out, value < 0 ? (uint32_t)(-(value + 1)) + 1U : (uint32_t)value,
                       10U, value < 0, width, pad);
            break;
        }
        case 'u': out_number(out, va_arg(args, uint32_t), 10U, false, width, pad); break;
        case 'x': out_number(out, va_arg(args, uint32_t), 16U, false, width, pad); break;
        case '%': out_char(out, '%'); break;
        default: /* unsupported: show it rather than misread args */
            out_char(out, '%');
            if(*fmt == '\0')
            {
                return;
            }
            out_char(out, *fmt);
            break;
        }
    }
}

/**
 * @brief Variadic wrapper around log_format().
 * @param[in,out] out Output buffer.
 * @param[in]     fmt Format string.
 */
static void log_printf(log_out_t *out, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_format(out, fmt, args);
    va_end(args);
}

void app_log(char level, const char *fmt, ...)
{
    if(!log_ready)
    {
        return;
    }
    char line[LOG_LINE_MAX];
    /* Leave room for CR LF at the end of a truncated line. */
    log_out_t out = {line, 0U, sizeof(line) - 2U};
    log_printf(&out, "[%7lu] " LOG_TAG " %c ", HAL_GetTick(), level);
    va_list args;
    va_start(args, fmt);
    log_format(&out, fmt, args);
    va_end(args);
    out.cap = sizeof(line);
    out_char(&out, '\r');
    out_char(&out, '\n');
    log_push(line, (uint32_t)out.len);
}

void app_log_flush(uint32_t timeout_ms)
{
    if(!log_ready)
    {
        return;
    }
    /* Poll with interrupts off: this also runs on the fault path, where
     * neither TXE nor the HAL tick advances, so the bound is a loop count
     * (about 4 cycles each at 216 MHz). */
    uint32_t loops   = timeout_ms * 50000U;
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    while((log_head != log_tail || (USART6->ISR & USART_ISR_TC) == 0U) && loops-- > 0U)
    {
        if(log_head != log_tail && (USART6->ISR & USART_ISR_TXE) != 0U)
        {
            USART6->TDR = (uint8_t)log_ring[log_tail & (LOG_RING_SIZE - 1U)];
            log_tail++;
        }
    }
    USART6->CR1 &= ~USART_CR1_TXEIE;
    __set_PRIMASK(primask);
}

uint32_t app_log_dropped(void) { return log_dropped; }

/** @brief USART6 TXE ISR: send one ring byte, or disable TXE when empty. */
void USART6_IRQHandler(void)
{
    if((USART6->ISR & USART_ISR_TXE) == 0U)
    {
        return;
    }
    if(log_tail != log_head)
    {
        USART6->TDR = (uint8_t)log_ring[log_tail & (LOG_RING_SIZE - 1U)];
        log_tail++;
    }
    else
    {
        USART6->CR1 &= ~USART_CR1_TXEIE;
    }
}
