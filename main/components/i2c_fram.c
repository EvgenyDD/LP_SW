#include "i2c_fram.h"
#include "driver/i2c.h"
#include "lp_config.h"
#include <string.h>

// FM24CL16B - 2KB I2C FRAM

#define I2C_FM24LC16B_ADDR 0x50 // base address

void i2c_fram_read(uint32_t addr, uint8_t *buf, uint32_t sz)
{
	uint8_t addr_lo = addr & 0xFF;
	_ESP_ERROR_CHECK(i2c_master_write_read_device(0, I2C_FM24LC16B_ADDR + (addr >> 8), &addr_lo, 1, buf, sz, 2));
}

void i2c_fram_write(uint32_t addr, const uint8_t *buf, uint32_t sz)
{
	uint8_t b[1 + sz];
	b[0] = addr & 0xFF;
	memcpy(&b[1], buf, sz);
	_ESP_ERROR_CHECK(i2c_master_write_to_device(0, I2C_FM24LC16B_ADDR + (addr >> 8), b, 1 + sz, 2));
}
