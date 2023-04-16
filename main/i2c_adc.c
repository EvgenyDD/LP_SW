#include "i2c_adc.h"
#include "dev_drivers.h"
#include "driver/i2c.h"

// #include "esp_rom_gpio.h"
// #include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// #include "hal/gpio_hal.h"
// #include <stdatomic.h>
// #include <sys/cdefs.h>

#include "esp_log.h"

#define I2C_ADS7828_ADDR 0x48

static void i2c_w_b(uint8_t value)
{
	ESP_ERROR_CHECK(i2c_master_write_to_device(0, I2C_ADS7828_ADDR, &value, 1, 2));
}

static uint16_t i2c_r_2b(void)
{
	uint8_t v[2];
	ESP_ERROR_CHECK(i2c_master_read_from_device(0, I2C_ADS7828_ADDR, v, 2, 2));
	return (v[0] << 8) | v[1];
}

void i2c_adc_init(void)
{
	i2c_w_b(8);
}

static uint8_t ch_lookup[8] = {0, 4, 1, 5, 2, 6, 3, 7};

uint16_t adc_get(uint8_t channel)
{
	uint16_t v = 0xFFFF;
	if(xSemaphoreTake(g_i2c_mutex, portMAX_DELAY))
	{
		i2c_w_b(0x8C | (ch_lookup[channel] << 4));
		// 6us
		asm("nop");
		asm("nop");
		asm("nop");
		asm("nop");
		asm("nop");
		v = i2c_r_2b();
		xSemaphoreGive(g_i2c_mutex);
	}
	return v;
}