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

	// sense key
	gpio_pad_select_gpio(34);
	gpio_set_direction(34, GPIO_MODE_INPUT);

	// Emergency button
	gpio_pad_select_gpio(35);
	gpio_set_direction(35, GPIO_MODE_INPUT);

	gpio_pad_select_gpio(16);
	gpio_set_direction(16, GPIO_MODE_OUTPUT);
}