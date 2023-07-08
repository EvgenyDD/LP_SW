#ifndef __I2C_ACCEL_H__
#define __I2C_ACCEL_H__

#include <stdint.h>

void i2c_accel_init(void);
void i2c_accel_get_data(uint8_t acc[3]);

#endif // __I2C_ACCEL_H__