#include "hw.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"

void hw_init(void)
{
	// sense key
	esp_rom_gpio_pad_select_gpio(34);
	gpio_set_direction(34, GPIO_MODE_INPUT);

	// Emergency button
	esp_rom_gpio_pad_select_gpio(35);
	gpio_set_direction(35, GPIO_MODE_INPUT);

	esp_rom_gpio_pad_select_gpio(16);
	gpio_set_direction(16, GPIO_MODE_OUTPUT);
}