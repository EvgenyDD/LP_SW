#include "imu.h"
#include "i2c_common.h"
#include "stm32f4xx.h"

#define LSM6DS3_ADDR 0X6A

#define LSM6DS3_TEST_PAGE 0X00
#define LSM6DS3_RAM_ACCESS 0X01
#define LSM6DS3_SENSOR_SYNC_TIME 0X04
#define LSM6DS3_SENSOR_SYNC_EN 0X05
#define LSM6DS3_FIFO_CTRL1 0X06
#define LSM6DS3_FIFO_CTRL2 0X07
#define LSM6DS3_FIFO_CTRL3 0X08
#define LSM6DS3_FIFO_CTRL4 0X09
#define LSM6DS3_FIFO_CTRL5 0X0A
#define LSM6DS3_ORIENT_CFG_G 0X0B
#define LSM6DS3_REFERENCE_G 0X0C
#define LSM6DS3_INT1_CTRL 0X0D
#define LSM6DS3_INT2_CTRL 0X0E
#define LSM6DS3_WHO_AM_I_REG 0X0F
#define LSM6DS3_CTRL1_XL 0X10
#define LSM6DS3_CTRL2_G 0X11
#define LSM6DS3_CTRL3_C 0X12
#define LSM6DS3_CTRL4_C 0X13
#define LSM6DS3_CTRL5_C 0X14
#define LSM6DS3_CTRL6_G 0X15
#define LSM6DS3_CTRL7_G 0X16
#define LSM6DS3_CTRL8_XL 0X17
#define LSM6DS3_CTRL9_XL 0X18
#define LSM6DS3_CTRL10_C 0X19
#define LSM6DS3_MASTER_CONFIG 0X1A
#define LSM6DS3_WAKE_UP_SRC 0X1B
#define LSM6DS3_TAP_SRC 0X1C
#define LSM6DS3_D6D_SRC 0X1D
#define LSM6DS3_STATUS_REG 0X1E
#define LSM6DS3_OUT_TEMP_L 0X20
#define LSM6DS3_OUT_TEMP_H 0X21
#define LSM6DS3_OUTX_L_G 0X22
#define LSM6DS3_OUTX_H_G 0X23
#define LSM6DS3_OUTY_L_G 0X24
#define LSM6DS3_OUTY_H_G 0X25
#define LSM6DS3_OUTZ_L_G 0X26
#define LSM6DS3_OUTZ_H_G 0X27
#define LSM6DS3_OUTX_L_XL 0X28
#define LSM6DS3_OUTX_H_XL 0X29
#define LSM6DS3_OUTY_L_XL 0X2A
#define LSM6DS3_OUTY_H_XL 0X2B
#define LSM6DS3_OUTZ_L_XL 0X2C
#define LSM6DS3_OUTZ_H_XL 0X2D
#define LSM6DS3_SENSORHUB1_REG 0X2E
#define LSM6DS3_SENSORHUB2_REG 0X2F
#define LSM6DS3_SENSORHUB3_REG 0X30
#define LSM6DS3_SENSORHUB4_REG 0X31
#define LSM6DS3_SENSORHUB5_REG 0X32
#define LSM6DS3_SENSORHUB6_REG 0X33
#define LSM6DS3_SENSORHUB7_REG 0X34
#define LSM6DS3_SENSORHUB8_REG 0X35
#define LSM6DS3_SENSORHUB9_REG 0X36
#define LSM6DS3_SENSORHUB10_REG 0X37
#define LSM6DS3_SENSORHUB11_REG 0X38
#define LSM6DS3_SENSORHUB12_REG 0X39
#define LSM6DS3_FIFO_STATUS1 0X3A
#define LSM6DS3_FIFO_STATUS2 0X3B
#define LSM6DS3_FIFO_STATUS3 0X3C
#define LSM6DS3_FIFO_STATUS4 0X3D
#define LSM6DS3_FIFO_DATA_OUT_L 0X3E
#define LSM6DS3_FIFO_DATA_OUT_H 0X3F
#define LSM6DS3_TIMESTAMP0_REG 0X40
#define LSM6DS3_TIMESTAMP1_REG 0X41
#define LSM6DS3_TIMESTAMP2_REG 0X42
#define LSM6DS3_STEP_COUNTER_L 0X4B
#define LSM6DS3_STEP_COUNTER_H 0X4C
#define LSM6DS3_FUNC_SRC 0X53
#define LSM6DS3_TAP_CFG1 0X58
#define LSM6DS3_TAP_THS_6D 0X59
#define LSM6DS3_INT_DUR2 0X5A
#define LSM6DS3_WAKE_UP_THS 0X5B
#define LSM6DS3_WAKE_UP_DUR 0X5C
#define LSM6DS3_FREE_FALL 0X5D
#define LSM6DS3_MD1_CFG 0X5E
#define LSM6DS3_MD2_CFG 0X5F

#define LSM6DS3_ADDR0_TO_RW_RAM 0x62
#define LSM6DS3_ADDR1_TO_RW_RAM 0x63
#define LSM6DS3_DATA_TO_WR_RAM 0x64
#define LSM6DS3_DATA_RD_FROM_RAM 0x65

#define LSM6DS3_RAM_SIZE 4096

#define LSM6DS3_SLV0_ADD 0x02
#define LSM6DS3_SLV0_SUBADD 0x03
#define LSM6DS3_SLAVE0_CONFIG 0x04
#define LSM6DS3_SLV1_ADD 0x05
#define LSM6DS3_SLV1_SUBADD 0x06
#define LSM6DS3_SLAVE1_CONFIG 0x07
#define LSM6DS3_SLV2_ADD 0x08
#define LSM6DS3_SLV2_SUBADD 0x09
#define LSM6DS3_SLAVE2_CONFIG 0x0A
#define LSM6DS3_SLV3_ADD 0x0B
#define LSM6DS3_SLV3_SUBADD 0x0C
#define LSM6DS3_SLAVE3_CONFIG 0x0D
#define LSM6DS3_DATAWRITE_SRC_MODE_SUB_SLV0 0x0E
#define LSM6DS3_CONFIG_PEDO_THS_MIN 0x0F
#define LSM6DS3_CONFIG_TILT_IIR 0x10
#define LSM6DS3_CONFIG_TILT_ACOS 0x11
#define LSM6DS3_CONFIG_TILT_WTIME 0x12
#define LSM6DS3_SM_STEP_THS 0x13
#define LSM6DS3_MAG_SI_XX 0x24
#define LSM6DS3_MAG_SI_XY 0x25
#define LSM6DS3_MAG_SI_XZ 0x26
#define LSM6DS3_MAG_SI_YX 0x27
#define LSM6DS3_MAG_SI_YY 0x28
#define LSM6DS3_MAG_SI_YZ 0x29
#define LSM6DS3_MAG_SI_ZX 0x2A
#define LSM6DS3_MAG_SI_ZY 0x2B
#define LSM6DS3_MAG_SI_ZZ 0x2C
#define LSM6DS3_MAG_OFFX_L 0x2D
#define LSM6DS3_MAG_OFFX_H 0x2E
#define LSM6DS3_MAG_OFFY_L 0x2F
#define LSM6DS3_MAG_OFFY_H 0x30
#define LSM6DS3_MAG_OFFZ_L 0x31
#define LSM6DS3_MAG_OFFZ_H 0x32

#define LSM6DS3_BW_XL_400Hz 0x00
#define LSM6DS3_BW_XL_200Hz 0x01
#define LSM6DS3_BW_XL_100Hz 0x02
#define LSM6DS3_BW_XL_50Hz 0x03

#define LSM6DS3_ACC_GYRO_ODR_XL_POWER_DOWN 0x00
#define LSM6DS3_ACC_GYRO_ODR_XL_13Hz 0x10
#define LSM6DS3_ACC_GYRO_ODR_XL_26Hz 0x20
#define LSM6DS3_ACC_GYRO_ODR_XL_52Hz 0x30
#define LSM6DS3_ACC_GYRO_ODR_XL_104Hz 0x40
#define LSM6DS3_ACC_GYRO_ODR_XL_208Hz 0x50
#define LSM6DS3_ACC_GYRO_ODR_XL_416Hz 0x60
#define LSM6DS3_ACC_GYRO_ODR_XL_833Hz 0x70
#define LSM6DS3_ACC_GYRO_ODR_XL_1660Hz 0x80
#define LSM6DS3_ACC_GYRO_ODR_XL_3330Hz 0x90
#define LSM6DS3_ACC_GYRO_ODR_XL_6660Hz 0xA0
#define LSM6DS3_ACC_GYRO_ODR_XL_13330Hz 0xB0

#define LSM6DS3_FS_XL_2g 0x00
#define LSM6DS3_FS_XL_16g 0x04
#define LSM6DS3_FS_XL_4g 0x08
#define LSM6DS3_FS_XL_8g 0x0C

#define LSM6DS3_FS_G_245dps 0x00
#define LSM6DS3_FS_G_500dps 0x04
#define LSM6DS3_FS_G_1000dps 0x08
#define LSM6DS3_FS_G_2000dps 0x0C

#define LSM6DS3_ODR_G_POWER_DOWN 0x00
#define LSM6DS3_ODR_G_13Hz 0x10
#define LSM6DS3_ODR_G_26Hz 0x20
#define LSM6DS3_ODR_G_52Hz 0x30
#define LSM6DS3_ODR_G_104Hz 0x40
#define LSM6DS3_ODR_G_208Hz 0x50
#define LSM6DS3_ODR_G_416Hz 0x60
#define LSM6DS3_ODR_G_833Hz 0x70
#define LSM6DS3_ODR_G_1660Hz 0x80
#define LSM6DS3TRC_ODR_G_3330Hz 0x90
#define LSM6DS3TRC_ODR_G_6660Hz 0xA0

#define LSM6DS3_BDU_BLOCK_UPDATE 0x40
#define LSM6DS3_IF_INC_ENABLED 0x04

imu_val_t imu_val;

#define CHK(x) \
	sts = x;   \
	if(sts) return sts;

static int sts = 0;

int imu_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4; // IMU_TILT
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	CHK(i2c_write(LSM6DS3_ADDR, (uint8_t[]){LSM6DS3_CTRL1_XL, LSM6DS3_BW_XL_50Hz | LSM6DS3_FS_XL_2g | LSM6DS3_ACC_GYRO_ODR_XL_52Hz}, 2, false));
	CHK(i2c_write(LSM6DS3_ADDR, (uint8_t[]){LSM6DS3_CTRL2_G, LSM6DS3_FS_G_245dps | LSM6DS3_ODR_G_52Hz}, 2, false));
	CHK(i2c_write(LSM6DS3_ADDR, (uint8_t[]){LSM6DS3_CTRL3_C, LSM6DS3_IF_INC_ENABLED}, 2, false));
	return 0;
}

int imu_read(void)
{
	uint8_t rxb[12];
	CHK(i2c_write(LSM6DS3_ADDR, (uint8_t[]){LSM6DS3_CTRL3_C, LSM6DS3_BDU_BLOCK_UPDATE | LSM6DS3_IF_INC_ENABLED}, 2, false));
	CHK(i2c_write(LSM6DS3_ADDR, (uint8_t[]){LSM6DS3_OUTX_L_G}, 1, true));
	CHK(i2c_read(LSM6DS3_ADDR, rxb, 12, false));

	imu_val.g[0] = (float)((int16_t)((uint16_t)rxb[1] << 8) + rxb[0]) * (1.0f / 32768.0f) * 245.0f;
	imu_val.g[1] = (float)((int16_t)((uint16_t)rxb[3] << 8) + rxb[2]) * (1.0f / 32768.0f) * 245.0f;
	imu_val.g[2] = (float)((int16_t)((uint16_t)rxb[5] << 8) + rxb[4]) * (1.0f / 32768.0f) * 245.0f;
	imu_val.xl[0] = (float)((int16_t)((uint16_t)rxb[7] << 8) + rxb[6]) * (1.0f / 32768.0f) * 2.0f;
	imu_val.xl[1] = (float)((int16_t)((uint16_t)rxb[9] << 8) + rxb[8]) * (1.0f / 32768.0f) * 2.0f;
	imu_val.xl[2] = (float)((int16_t)((uint16_t)rxb[11] << 8) + rxb[10]) * (1.0f / 32768.0f) * 2.0f;

	return 0;
}