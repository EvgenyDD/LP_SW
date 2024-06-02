#ifndef CRC16_CCITT_H__
#define CRC16_CCITT_H__

#include <stdint.h>

/**
 * `x^16 + x^12 + x^5 + 1`
 */

/**
 * Calculate CRC sum on block of data.
 *
 * @param data Pointer to block of data.
 * @param len Length of data in bytes;
 * @param crc Initial value (zero for xmodem). If block is split into
 * multiple segments, previous CRC is used as initial.
 */
uint16_t crc16_ccitt(const uint8_t *data, uint32_t len, uint16_t crc);

#endif // CRC16_CCITT_H__