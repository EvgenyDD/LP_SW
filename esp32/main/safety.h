#ifndef SAFETY_H__
#define SAFETY_H__

#include <stdbool.h>
#include <stdint.h>

void safety_init(void);
void safety_loop(uint32_t diff_ms);
void safety_reset_lock(void);

void safety_enable_power_head(void);
void safety_disable_power_head(void);

void safety_enable_lpc(void);
void safety_disable_lpc(void);

void safety_lsr_set_used_by_stm(bool state);
uint8_t safety_lsr_is_used_by_esp(void);
uint8_t safety_is_pwr_enabled(void);

#endif // SAFETY_H__