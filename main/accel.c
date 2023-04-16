#include "accel.h"
#include "dev_drivers.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_rom_gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/emac_hal.h"
#include "hal/gpio_hal.h"
#include <stdatomic.h>
#include <sys/cdefs.h>

#define ACCEL_THRS 3

#define STK8312_I2C_ADDR 0x3D

#define STK8312_REG_X 0x00
#define STK8312_REG_Y 0x01
#define STK8312_REG_Z 0x02

#define STK8312_REG_MODE 0x07
#define STK8312_REG_SR 0x08
#define STK8312_REG_STH 0x13
#define STK8312_REG_SWRST 0x20

atomic_bool pos_fix_failure = false;
static int8_t imu_acc_fix[3];

void write_reg(uint8_t reg, uint8_t value)
{
	uint8_t tx_arr[2];
	tx_arr[0] = reg;
	tx_arr[1] = value;
	ESP_ERROR_CHECK(i2c_master_write_to_device(0, STK8312_I2C_ADDR, tx_arr, 2, 2));
}

uint8_t read_reg(uint8_t reg)
{
	uint8_t b;
	ESP_ERROR_CHECK(i2c_master_write_read_device(0, STK8312_I2C_ADDR, &reg, 1, &b, 1, 2));
	return b;
}

void accel_reset_fixture(void)
{
	if(xSemaphoreTake(g_i2c_mutex, portMAX_DELAY))
	{
		accel_get_data(imu_acc_fix);
		pos_fix_failure = false;
		ESP_LOGI("", "New 3D pos fixture: %d %d %d", imu_acc_fix[0], imu_acc_fix[1], imu_acc_fix[2]);
		xSemaphoreGive(g_i2c_mutex);
	}
}

void accel_init(void)
{
	write_reg(STK8312_REG_SWRST, 0);
	vTaskDelay(5);
	write_reg(STK8312_REG_STH, (1 << 6));
	write_reg(STK8312_REG_MODE, (1 << 0));
	write_reg(STK8312_REG_SR, 0);
	vTaskDelay(1);

	uint8_t reg = 0;
	uint8_t b;
	for(uint32_t i = 0; i < 128; i++)
		if((i2c_master_write_read_device(0, i, &reg, 1, &b, 1, 2)) == 0)
			ESP_LOGI("", "ACK x%x", i);
}

void accel_task(void *pvParameter)
{
	if(!xSemaphoreTake(g_i2c_mutex, portMAX_DELAY)) return;
	{
		accel_init();
		xSemaphoreGive(g_i2c_mutex);
	}

	vTaskDelay(5);

	accel_reset_fixture();

	while(1)
	{
		int8_t imu_acc[3];
		if(xSemaphoreTake(g_i2c_mutex, portMAX_DELAY))
		{
			accel_get_data(imu_acc);
			xSemaphoreGive(g_i2c_mutex);

			if(!pos_fix_failure && (imu_acc[0] || imu_acc[1] || imu_acc[2]))
			{
				if(abs(imu_acc[0] - imu_acc_fix[0]) > ACCEL_THRS ||
				   abs(imu_acc[1] - imu_acc_fix[1]) > ACCEL_THRS ||
				   abs(imu_acc[2] - imu_acc_fix[2]) > ACCEL_THRS)
				{
					pos_fix_failure = true;
					ESP_LOGE("", "3D pos fixture failure! %d %d %d", imu_acc[0], imu_acc[1], imu_acc[2]);
				}
			}
		}
		vTaskDelay(5);
	}
}

void accel_get_data(int8_t acc[3])
{
	uint8_t wr = STK8312_REG_X;
	i2c_master_write_read_device(0, STK8312_I2C_ADDR, &wr, 1, (uint8_t *)acc, 3, 2);
}