/**
 * @file    uart3.h
 * @brief   USART3 HWLINK transport on the ST-LINK VCP.
 * @author  Sandesh Ghimire <sandesh@soccentric.com>
 * @date    2026-11-03
 * @target  NUCLEO-F767ZI (PD8 TX, PD9 RX)
 * @srs     COM-001, COM-002, DCON-004
 */

#ifndef UART3_H
#define UART3_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @defgroup uart3 USART3 driver
 * @brief DMA ring receive and DMA transmit on USART3.
 * @{
 */

#define APP_UART_DEFAULT_BAUD 115200U /**< Power-on baud rate. */
#define APP_UART_FAST_BAUD    921600U /**< Optional fast baud rate (COM-001). */

/**
 * @brief Start USART3 at 115200 8N1 with circular DMA receive.
 * @pre Clocks configured; scheduler not yet started.
 * @return true on success.
 */
bool app_uart3_init(void);

/**
 * @brief Switch to @ref APP_UART_DEFAULT_BAUD or @ref APP_UART_FAST_BAUD.
 * @details Restarts the receiver and drops unread bytes. On failure the old
 *          rate is restored.
 * @pre The Ack for the request has been sent (COM-001).
 * @param[in] baud New rate in bit/s.
 * @return true when @p baud is active.
 * @retval false Unsupported rate or re-init failed.
 */
bool app_uart3_set_baud(uint32_t baud);

/**
 * @brief Get the active baud rate.
 * @return Rate in bit/s.
 */
uint32_t app_uart3_baud(void);

/**
 * @brief Copy received bytes out of the DMA ring.
 * @details If the DMA lapped the reader, resyncs to the write position and
 *          returns 0.
 * @param[out] dst     Destination buffer.
 * @param[in]  max_len Capacity of @p dst in bytes.
 * @return Number of bytes copied.
 * @note Single consumer (Comms task); not ISR-safe.
 */
uint32_t app_uart3_rx_read(uint8_t *dst, uint32_t max_len);

/**
 * @brief Check that the DMA receiver is armed (HLT-001).
 * @return true while USART3 RX DMA is running.
 */
bool app_uart3_rx_running(void);

/**
 * @brief Re-arm the circular DMA receiver after an error or abort.
 * @return true when the receiver is running afterwards.
 * @note Comms task only; no-op when already running.
 */
bool app_uart3_rx_restart(void);

/**
 * @brief Transmit one frame by DMA and wait for the last stop bit.
 * @details Blocks the calling task on its task notification; other tasks run.
 *
 * @param[in] data       Bytes to send; must stay valid until return.
 * @param[in] len        Number of bytes; 0 returns true at once.
 * @param[in] timeout_ms DMA completion timeout in ms.
 * @return true when sent.
 * @retval false DMA start failed or timed out.
 * @warning One calling task only; uses that task's notification value.
 */
bool app_uart3_send(const uint8_t *data, uint16_t len, uint32_t timeout_ms);

/**
 * @brief Wake the Comms task after receive activity.
 * @note Called from the USART3/DMA ISRs; implemented in platform.c.
 */
void comms_post_rx_from_isr(void);

/** @} */

#endif /* UART3_H */
