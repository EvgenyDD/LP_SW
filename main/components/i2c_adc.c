#include "i2c_adc.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lp_config.h"

#define I2C_ADS7828_ADDR 0x48

#define NTC_COEF 3380.0f
// NTC -> lo side
#define NTC_RES_LO_SIDE(adc_val) ((adc_val * 10000.0f) / (5405 /* vref 2.5 pullup 3.3 */ - adc_val))
#define NTC_TEMP_LO_SIDE(adc_raw, coef) (1.0f / ((logf_fast(NTC_RES_LO_SIDE(adc_raw) / 10000.0f) / coef) + (1.0 / 298.15f)) - 273.15f)

adc_acq_t adc_val = {0};

static const uint8_t ch_lookup[8] = {0, 4, 1, 5, 2, 6, 3, 7};

static void i2c_w_b(uint8_t value)
{
	_ESP_ERROR_CHECK(i2c_master_write_to_device(0, I2C_ADS7828_ADDR, &value, 1, 2));
}

static uint16_t i2c_r_2b(void)
{
	uint8_t v[2];
	_ESP_ERROR_CHECK(i2c_master_read_from_device(0, I2C_ADS7828_ADDR, v, 2, 2));
	return (v[0] << 8) | v[1];
}

void i2c_adc_init(void)
{
	i2c_w_b(8);
}

int read_channel(uint8_t channel)
{
	i2c_w_b(0x8C | (ch_lookup[channel] << 4));
	// 6us
	asm("nop");
	asm("nop");
	asm("nop");
	asm("nop");
	asm("nop");
	return i2c_r_2b();
}

inline int __float_as_int(float in)
{
	union fi
	{
		int i;
		float f;
	} conv;
	conv.f = in;
	return conv.i;
}

inline float __int_as_float(int in)
{
	union fi
	{
		int i;
		float f;
	} conv;
	conv.i = in;
	return conv.f;
}

/* natural log on [0x1.f7a5ecp-127, 0x1.fffffep127]. Maximum relative error 9.4529e-5 */
inline float logf_fast(float a)
{
	float m, r, s, t, i, f;
	int32_t e;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
	e = (__float_as_int(a) - 0x3f2aaaab) & 0xff800000;
#pragma GCC diagnostic pop

	m = __int_as_float(__float_as_int(a) - e);
	i = (float)e * 1.19209290e-7f; // 0x1.0p-23
	/* m in [2/3, 4/3] */
	f = m - 1.0f;
	s = f * f;
	/* Compute log1p(f) for f in [-1/3, 1/3] */
	r = (0.230836749f * f - 0.279208571f); // 0x1.d8c0f0p-3, -0x1.1de8dap-2
	t = (0.331826031f * f - 0.498910338f); // 0x1.53ca34p-2, -0x1.fee25ap-2
	r = (r * s + t);
	r = (r * s + f);
	r = (i * 0.693147182f + r); // 0x1.62e430p-1 // log(2)
	return r;
}

void i2c_adc_read(void)
{
	// CURRENT sensor
	// ina180a2
	// 0.04198936409*x+0.4741118019 (W) [172W max]
	adc_val.i_p = read_channel(0) * 17443 / 10000; // 1.7442874585731727; // 2.5 / (4095 * 50 * 0.007) * 1000.0

	// 4.14612523+0.01133214557*x+(-1.176550817E-6)*x^2 -> -24V
	int ch = read_channel(1);
	adc_val.v_n = ch * ch * -12 / 10000 + ch * 1133 / 100 + 4146;

	// 0.0105615645*x-0.01300610299 -> U meters
	adc_val.v_p = read_channel(2) * 1056 / 100;
	adc_val.v_i = read_channel(3) * 1056 / 100;

	// thermistors: B25/50=3380K 10k NTCM-HP-10K-1% SR PASSIVES
	adc_val.t_drv = (uint16_t)(10.0 * NTC_TEMP_LO_SIDE(read_channel(4), NTC_COEF));
	adc_val.t_lsr_head = (uint16_t)(10.0 * NTC_TEMP_LO_SIDE(read_channel(5), NTC_COEF));
	adc_val.t_inv_p = (uint16_t)(10.0 * NTC_TEMP_LO_SIDE(read_channel(6), NTC_COEF));
	adc_val.t_inv_n = (uint16_t)(10.0 * NTC_TEMP_LO_SIDE(read_channel(7), NTC_COEF));
}