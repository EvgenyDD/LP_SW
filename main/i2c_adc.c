#include "i2c_adc.h"
#include "dev_drivers.h"
#include "driver/i2c.h"
// #include "esp_log.h"
// #include "esp_rom_gpio.h"
// #include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// #include "hal/gpio_hal.h"
// #include <stdatomic.h>
// #include <sys/cdefs.h>

#define I2C_ADS7828_ADDR 0x48

static void write(uint8_t value)
{
	ESP_ERROR_CHECK(i2c_master_write_to_device(0, I2C_ADS7828_ADDR, &value, 1, 2));
}

static uint8_t read_val(void)
{
	uint16_t v;
	ESP_ERROR_CHECK(i2c_master_read_from_device(0, I2C_ADS7828_ADDR, (uint8_t *)&v, 2, 2));
	return v;
}

void i2c_adc_init(void)
{
	write(8);
}

uint16_t adc_get(uint8_t channel)
{
	uint16_t v = 0xFFFF;
	if(xSemaphoreTake(g_i2c_mutex, portMAX_DELAY))
	{
		write(0x8C & (channel << 4));
		// 6us
		vTaskDelay(1);
		v = read_val();
		xSemaphoreGive(g_i2c_mutex);
	}
	return v;
}