#ifndef I2C_ADC_H__
#define I2C_ADC_H__

#include <stdint.h>

void i2c_adc_init(void);
uint16_t adc_get(uint8_t channel);

#endif // I2C_ADC_H__