#ifndef __BUTTONS_H__
#define __BUTTONS_H__

#include <stdint.h>

void buttons_init(void);
uint8_t buttons_get(void);

void lsr_volt_enable(void);
void lsr_volt_disable(void);

#endif // __BUTTONS_H__