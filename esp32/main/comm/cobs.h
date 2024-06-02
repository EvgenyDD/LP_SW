#ifndef __COBS_H__
#define __COBS_H__

#include <stdint.h>

unsigned int cobs_encode(const uint8_t *ptr, uint16_t length, volatile uint8_t *dst);
unsigned int cobs_decode(const uint8_t *ptr, uint16_t length, uint8_t *dst);

#endif //__COBS_H__
