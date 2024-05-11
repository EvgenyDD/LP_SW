#ifndef SAFETY_H__
#define SAFETY_H__

#include <stdbool.h>
#include <stdint.h>

bool safety_check_error(void);
void safety_loop(uint32_t diff_ms);
void safety_reset_lock(void);

void fan_init(void);
void fan_update(uint8_t value);
void fan_track(uint32_t diff_ms);
float fan_get_vel(void);

void safety_enable(void);
void safety_disable(void);

#endif // SAFETY_H__