#include "i2c_accel.h"
#include "driver/i2c.h"
// #include "esp_log.h"
#include "esp_rom_gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "hal/emac_hal.h"
#include "hal/gpio_hal.h"
#include <sys/cdefs.h>

#define ACCEL_THRS 5

#define STK8312_I2C_ADDR 0x3D

#define STK8312_REG_X 0x00
#define STK8312_REG_Y 0x01
#define STK8312_REG_Z 0x02

#define STK8312_REG_MODE 0x07
#define STK8312_REG_SR 0x08
#define STK8312_REG_STH 0x13
#define STK8312_REG_SWRST 0x20

static void write_reg(uint8_t reg, uint8_t value)
{
	uint8_t tx_arr[2];
	tx_arr[0] = reg;
	tx_arr[1] = value;
	ESP_ERROR_CHECK(i2c_master_write_to_device(0, STK8312_I2C_ADDR, tx_arr, 2, 2));
}

static uint8_t read_reg(uint8_t reg)
{
	uint8_t b;
	ESP_ERROR_CHECK(i2c_master_write_read_device(0, STK8312_I2C_ADDR, &reg, 1, &b, 1, 2));
	return b;
}

void i2c_accel_init(void)
{
	write_reg(STK8312_REG_SWRST, 0);
	vTaskDelay(5);
	write_reg(STK8312_REG_STH, (1 << 6));
	write_reg(STK8312_REG_MODE, (1 << 0));
	write_reg(STK8312_REG_SR, 0);
	read_reg(0);
	vTaskDelay(1);
}

void i2c_accel_get_data(uint8_t acc[3])
{
	uint8_t wr = STK8312_REG_X;
	// i2c_master_write_read_device(0, STK8312_I2C_ADDR, &wr, 1, acc, 3, 2);
	i2c_master_write_read_device(0, STK8312_I2C_ADDR, &wr, 1, acc, 1, 2);
	wr = STK8312_REG_Y;
	i2c_master_write_read_device(0, STK8312_I2C_ADDR, &wr, 1, &acc[1], 1, 2);
	wr = STK8312_REG_Z;
	i2c_master_write_read_device(0, STK8312_I2C_ADDR, &wr, 1, &acc[2], 1, 2);
}