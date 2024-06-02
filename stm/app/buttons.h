#ifndef BUTTONS_H__
#define BUTTONS_H__

#include "debounce.h"
#include <stdint.h>

void buttons_init(void);
void buttons_poll(uint32_t diff_ms);

extern debounce_t btn_act[3], btn_jl, btn_jr, btn_jok, btn_ju, btn_jd, btn_emcy, sd_detect_debounce;

#endif // BUTTONS_H__