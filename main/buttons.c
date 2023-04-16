#include "buttons.h"
// #include "dev_drivers.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <string.h>

// TCA6408A

#define IO_EXP_I2C_ADDR 0x21

#define REG_IN 0x00
#define REG_OUT 0x01
#define REG_POL_INV 0x02
#define REG_CFG 0x03

#define IO_CFG_EMCY 0xFE // laser voltage OFF
#define IO_CFG_NRML 0xFF // laser voltage ON

void buttons_init(void)
{
	ESP_ERROR_CHECK(i2c_master_write_to_device(0, IO_EXP_I2C_ADDR, (uint8_t[]){REG_POL_INV, 0xFF}, 2, 2));
	ESP_ERROR_CHECK(i2c_master_write_to_device(0, IO_EXP_I2C_ADDR, (uint8_t[]){REG_OUT, 0x00}, 2, 2));
	ESP_ERROR_CHECK(i2c_master_write_to_device(0, IO_EXP_I2C_ADDR, (uint8_t[]){REG_CFG, IO_CFG_NRML}, 2, 2));
}

uint8_t buttons_get(void)
{
	uint8_t value;
	ESP_ERROR_CHECK(i2c_master_write_read_device(0, IO_EXP_I2C_ADDR, (uint8_t[]){REG_IN}, 1, &value, 1, 2));
	return value;
}

void lsr_volt_enable(void) // don't forget to set FAN PWM >350
{
	ESP_ERROR_CHECK(i2c_master_write_to_device(0, IO_EXP_I2C_ADDR, (uint8_t[]){REG_CFG, IO_CFG_NRML}, 2, 2));
}

void lsr_volt_disable(void)
{
	ESP_ERROR_CHECK(i2c_master_write_to_device(0, IO_EXP_I2C_ADDR, (uint8_t[]){REG_OUT, 0x00}, 2, 2));
	ESP_ERROR_CHECK(i2c_master_write_to_device(0, IO_EXP_I2C_ADDR, (uint8_t[]){REG_CFG, IO_CFG_EMCY}, 2, 2));
}