#ifndef __I2C_FRAM_H__
#define __I2C_FRAM_H__

#include <stdint.h>

void i2c_fram_read(uint32_t addr, uint8_t *buf, uint32_t sz);
void i2c_fram_write(uint32_t addr, const uint8_t *buf, uint32_t sz);

#endif // __I2C_FRAM_H__