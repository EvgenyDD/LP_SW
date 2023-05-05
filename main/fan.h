#ifndef __FAN_H__
#define __FAN_H__

#include <stdint.h>

void fan_init(void);
void fan_set_level(uint8_t value);
uint16_t fan_get_level(void);

#endif // __FAN_H__