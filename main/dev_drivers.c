#include "dev_drivers.h"
#include "driver/i2c.h"
#include "esp_eth.h"
#include "esp_rom_gpio.h"
#include "hal/emac_hal.h"
#include "hal/gpio_hal.h"
#include <stdatomic.h>

SemaphoreHandle_t g_i2c_mutex;

static i2c_config_t i2c_conf = {
	.mode = I2C_MODE_MASTER,
	.sda_io_num = 33,
	.scl_io_num = 32,
	.sda_pullup_en = GPIO_PULLUP_ENABLE,
	.scl_pullup_en = GPIO_PULLUP_ENABLE,
	.master.clk_speed = 100000,
};

void dev_drivers_init(void)
{
	i2c_param_config(0, &i2c_conf);
	i2c_driver_install(0, I2C_MODE_MASTER, 0, 0, 0);
	g_i2c_mutex = xSemaphoreCreateMutex();
}
