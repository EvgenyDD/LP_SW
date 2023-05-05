#ifndef I2C_ADC_H__
#define I2C_ADC_H__

#include <stdint.h>

void i2c_adc_init(void);
uint16_t i2c_adc_get(uint8_t channel);
void i2c_adc_read(void);

extern uint16_t t_lsr_head, t_drv, t_inv_p, t_inv_n;
extern uint16_t v_p, v_n, v_i;
extern uint16_t i_p;

#endif // I2C_ADC_H__