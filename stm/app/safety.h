#ifndef SAFETY_H__
#define SAFETY_H__

#include <stdbool.h>
#include <stdint.h>

bool safety_check_error(void);
void safety_loop(uint32_t diff_ms);
void safety_reset_ser_to(void);

void safety_reset_lock(void);
void safety_enable_power_and_ctrl(void);
void safety_disable_power_and_ctrl(void);
void safety_enable_power(void);
void safety_disable(void);

void fan_init(void);
void fan_update(uint8_t value);
void fan_track(uint32_t diff_ms);
float fan_get_vel(void);

uint8_t safety_lsr_is_used_by_stm(void);
void safety_lsr_set_used_by_esp(bool state);
uint8_t safety_is_pwr_enabled(void);

#endif // SAFETY_H__