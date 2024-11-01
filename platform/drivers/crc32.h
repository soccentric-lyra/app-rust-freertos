/**
 * @file    crc32.h
 * @brief   CRC-32/ISO-HDLC (zlib polynomial), bitwise, no table.
 * @author  Sandesh Ghimire <sandesh@soccentric.com>
 * @date    2026-11-03
 * @target  STM32F767ZIT6
 * @srs     HWLINK LNK-003/004, BOOT Appendix B
 */

#ifndef CRC32_H
#define CRC32_H

#include <stdint.h>
#include <stddef.h>

/**
 * @defgroup crc32 CRC-32
 * @brief CRC-32/ISO-HDLC, identical to zlib's crc32().
 * @{
 */

/**
 * @brief Compute CRC-32/ISO-HDLC over a buffer.
 * @param[in] data Bytes to check.
 * @param[in] len  Length of @p data in bytes.
 * @return CRC-32 (init and final XOR 0xFFFFFFFF).
 * @note Reentrant; safe from any context.
 */
uint32_t app_crc32(const void *data, size_t len);

/** @} */

#endif /* CRC32_H */
