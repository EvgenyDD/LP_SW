#ifndef FRAM_H__
#define FRAM_H__

#include <stdint.h>

int fram_read(uint32_t addr, uint8_t *buf, uint32_t sz);
int fram_write(uint32_t addr, const uint8_t *buf, uint32_t sz);

#endif // FRAM_H__