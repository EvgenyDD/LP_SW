#include "accel.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_rom_gpio.h"
#include "esp_timer.h"
#include "eth_smi_iic.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "hal/emac_hal.h"
#include "hal/gpio_hal.h"
#include <stdatomic.h>
#include <sys/cdefs.h>

// unknown accel: reg 0x20 bits: 4/5/6/7

void write_reg(uint8_t reg, uint8_t value)
{
	uint8_t tx_arr[2];
	tx_arr[0] = reg;
	tx_arr[1] = value;
	i2c_master_write_to_device(0, MPU6050_I2C_ADDRESS, tx_arr, 2, 2);
}

uint8_t read_reg(uint8_t reg)
{
	uint8_t b;
	i2c_master_write_read_device(0, MPU6050_I2C_ADDRESS, &reg, 1, &b, 1, 2);
	return b;
}

void imu_init(void)
{
	write_reg(MPU6050_PWR_MGMT_1, 0); // wakeup the chip
	// uint8_t chipid = read_reg(MPU6050_WHO_AM_I); // read chip id

	vTaskDelay(5);

	write_reg(MPU6050_PWR_MGMT_1, 0); // internal 20MHz oscillator

	uint8_t tmp = read_reg(MPU6050_PWR_MGMT_2) & (~(0x38 | 0x07)); // enable acc/gyro xyz
	write_reg(MPU6050_PWR_MGMT_2, tmp);
	write_reg(MPU6050_ACCEL_CONFIG, 0x00 << 3 /* acc +-2g */);
	write_reg(MPU6050_GYRO_CONFIG, 0x03 << 3 /* gyro 2000dps*/);
}

void imu_get_acc(int16_t acc[3])
{
	uint8_t tmp[6] = {0};

	uint8_t wr = MPU6050_ACCEL_XOUT_H;
	i2c_master_write_read_device(0, MPU6050_I2C_ADDRESS, &wr, 1, tmp, 6, 2);

	acc[0] = (int16_t)((tmp[0] << 8) | tmp[1]);
	acc[1] = (int16_t)((tmp[2] << 8) | tmp[3]);
	acc[2] = (int16_t)((tmp[4] << 8) | tmp[5]);
}