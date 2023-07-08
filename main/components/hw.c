#include "hw.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp32/pm.h"
#include "esp_log.h"
#include "esp_pm.h"

void hw_init(void)
{
	esp_pm_config_esp32_t pm_config = {
		.max_freq_mhz = 240,
		.min_freq_mhz = 240,
		.light_sleep_enable = false};
	ESP_ERROR_CHECK(esp_pm_configure(&pm_config));

	static i2c_config_t i2c_conf = {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = 33,
		.scl_io_num = 32,
		.sda_pullup_en = GPIO_PULLUP_ENABLE,
		.scl_pullup_en = GPIO_PULLUP_ENABLE,
		.master.clk_speed = 400000,
	};
	i2c_param_config(0, &i2c_conf);
	i2c_driver_install(0, I2C_MODE_MASTER, 0, 0, 0);

// enumerate I2C devices
#if 0
	uint8_t reg = 0;
	uint8_t b;
	for(uint32_t i = 0; i < 128; i++)
		if((i2c_master_write_read_device(0, i, &reg, 1, &b, 1, 2)) == 0)
			ESP_LOGI("", "ACK x%x", i);
#endif

	// Emergency button
	gpio_pad_select_gpio(34);
	gpio_set_direction(34, GPIO_MODE_INPUT);
}