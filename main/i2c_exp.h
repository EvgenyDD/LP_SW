#ifndef __I2C_EXP_H__
#define __I2C_EXP_H__

#include "debounce.h"
#include <stdint.h>

void i2c_exp_init(void);
void buttons_read(int32_t time_diff);

void lsr_volt_enable(void);
void lsr_volt_disable(void);

extern button_ctrl_t btn_emcy, btn_j_l, btn_j_r, btn_j_press, btn_l, btn_r;

#endif // __I2C_EXP_H__