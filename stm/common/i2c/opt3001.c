#include "opt3001.h"
#include "i2c_common.h"

#define OPT3001_ADDR 0x44

#define OPT3001_REG_RESULT 0x00			 /**< Result register */
#define OPT3001_REG_CONFIG 0x01			 /**< Configuration register */
#define OPT3001_REG_LOW_LIMIT 0x02		 /**< Low Limit register */
#define OPT3001_REG_HIGH_LIMIT 0x03		 /**< High Limit register */
#define OPT3001_REG_MANUFACTURER_ID 0x7E /**< Manufacturer ID register */
#define OPT3001_REG_DEVICE_ID 0x7F		 /**< Device ID register */

#define OPT3001_CFG_RN_FSR (0xC0) /**< Automatic full-scale setting mode */

#define OPT3001_CONFIG_M_SHUTDOWN (0 << 1)	 /**< Shutdown mode */
#define OPT3001_CONFIG_M_SINGLE (1 << 1)	 /**< Single-shot mode */
#define OPT3001_CONFIG_M_CONTINUOUS (2 << 1) /**< Continuous mode (also 0x3) */

#define OPT3001_CFG_OVF (1 << 8) /**< Overflow flag field */
#define OPT3001_CFG_CRF (1 << 7) /**< Conversion ready field */
#define OPT3001_CFG_FH (1 << 6)	 /**< Flag high field */
#define OPT3001_CFG_FL (1 << 5)	 /**< Flag low field */
#define OPT3001_CFG_L (1 << 4)	 /**< Latch field */
#define OPT3001_CFG_POL (1 << 3) /**< Polarity field */
#define OPT3001_CFG_ME (1 << 2)	 /**< Mask exponent field */

float opt3001_lux = 0;

#define CHK(x) \
	sts = x;   \
	if(sts) return sts;

static int sts = 0;

int opt3001_init(void)
{
	// buffer[0] = OPT3001_REG_CONFIG;
	// buffer[1] = 0xc8;
	// buffer[2] = 0x10;

	// if(ft260_i2c_write_request(ft260, ctx->address, buffer, 3, true) != 0)
	// 	return -1;
	CHK(i2c_write(OPT3001_ADDR, (uint8_t[]){OPT3001_REG_CONFIG, OPT3001_CFG_RN_FSR | OPT3001_CONFIG_M_CONTINUOUS, OPT3001_CFG_L}, 3, false));
	return 0;
}

int opt3001_read(void)
{
	uint8_t rxb[2];
	CHK(i2c_write(OPT3001_ADDR, (uint8_t[]){OPT3001_REG_RESULT}, 1, true));
	CHK(i2c_read(OPT3001_ADDR, rxb, 2, false));

	uint16_t r = rxb[0] << 8 | rxb[1];
	opt3001_lux = ((1 << (r >> 12)) * (r & 0xfff)) * 0.01f;

	return 0;
}