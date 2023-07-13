#include "i2c_exp.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "hal/gpio_hal.h"
#include "lp_config.h"
#include <string.h>

// TCA6408A

#define IO_EXP_I2C_ADDR 0x21

#define REG_IN 0x00
#define REG_OUT 0x01
#define REG_POL_INV 0x02
#define REG_CFG 0x03

#define IO_CFG_EMCY 0xFE // laser voltage OFF
#define IO_CFG_NRML 0xFF // laser voltage ON

button_ctrl_t btn_emcy, btn_j_l, btn_j_r, btn_j_press, btn_l, btn_r;

void i2c_exp_init(void)
{
	debounce_init(&btn_emcy, 1);
	debounce_init(&btn_j_l, 1);
	debounce_init(&btn_j_r, 1);
	debounce_init(&btn_j_press, 1);
	debounce_init(&btn_l, 1);
	debounce_init(&btn_r, 1);

	_ESP_ERROR_CHECK(i2c_master_write_to_device(0, IO_EXP_I2C_ADDR, (uint8_t[]){REG_POL_INV, 0xFF}, 2, 2));
	_ESP_ERROR_CHECK(i2c_master_write_to_device(0, IO_EXP_I2C_ADDR, (uint8_t[]){REG_OUT, 0x00}, 2, 2));
	_ESP_ERROR_CHECK(i2c_master_write_to_device(0, IO_EXP_I2C_ADDR, (uint8_t[]){REG_CFG, IO_CFG_NRML}, 2, 2));
}

void lsr_volt_enable(void) // don't forget to set FAN PWM >350
{
	_ESP_ERROR_CHECK(i2c_master_write_to_device(0, IO_EXP_I2C_ADDR, (uint8_t[]){REG_CFG, IO_CFG_NRML}, 2, 2));
}

void lsr_volt_disable(void)
{
	_ESP_ERROR_CHECK(i2c_master_write_to_device(0, IO_EXP_I2C_ADDR, (uint8_t[]){REG_OUT, 0x00}, 2, 2));
	_ESP_ERROR_CHECK(i2c_master_write_to_device(0, IO_EXP_I2C_ADDR, (uint8_t[]){REG_CFG, IO_CFG_EMCY}, 2, 2));
}

void buttons_read(int32_t time_diff)
{
	uint8_t btn;
	_ESP_ERROR_CHECK(i2c_master_write_read_device(0, IO_EXP_I2C_ADDR, (uint8_t[]){REG_IN}, 1, &btn, 1, 2));
	debounce_cb(&btn_emcy, gpio_ll_get_level(&GPIO, 34) == 0, time_diff);
	debounce_cb(&btn_j_l, btn & (1 << 7), time_diff);
	debounce_cb(&btn_j_r, btn & (1 << 3), time_diff);
	debounce_cb(&btn_j_press, btn & (1 << 6), time_diff);
	debounce_cb(&btn_l, btn & (1 << 5), time_diff);
	debounce_cb(&btn_r, btn & (1 << 4), time_diff);
}