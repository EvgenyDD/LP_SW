#ifndef __I2C_ACCEL_H__
#define __I2C_ACCEL_H__

#include <stdint.h>

void i2c_accel_init(void);
void i2c_accel_get_data(uint8_t acc[3]);
void i2c_accel_read(void);

#endif // __I2C_ACCEL_H__