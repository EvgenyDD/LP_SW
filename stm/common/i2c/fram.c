#include "fram.h"
#include "error.h"
#include "i2c_common.h"
#include <string.h>

// FM24CL16B - 2KB I2C FRAM

#define I2C_FM24LC16B_ADDR 0x50 // base address

#define CHK(x)                      \
	sts = x;                        \
	if(sts)                         \
	{                               \
		error_set(ERR_STM_FRAM, 1); \
		return sts;                 \
	}

static int sts = 0;

int fram_read(uint32_t addr, uint8_t *buf, uint32_t sz)
{
	uint8_t addr_lo = addr & 0xFF;
	CHK(i2c_write(I2C_FM24LC16B_ADDR + (addr >> 8), &addr_lo, 1, true));
	CHK(i2c_read(I2C_FM24LC16B_ADDR + (addr >> 8), buf, sz, false));
	return 0;
}

int fram_write(uint32_t addr, const uint8_t *buf, uint32_t sz)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvla"
	uint8_t b[1 + sz];
#pragma GCC diagnostic pop

	b[0] = addr & 0xFF;
	memcpy(&b[1], buf, sz);
	return i2c_write(I2C_FM24LC16B_ADDR + (addr >> 8), b, 1 + sz, false);
}
