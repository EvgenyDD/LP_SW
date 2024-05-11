#ifndef SAFETY_H__
#define SAFETY_H__

#include <stdbool.h>
#include <stdint.h>

void safety_init(void);
void safety_loop(uint32_t diff_ms);
void safety_reset_lock(void);

bool safety_check_error(void);
bool safety_is_locked(void);

#endif // SAFETY_H__