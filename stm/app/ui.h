#ifndef UI_H__
#define UI_H__

#include <stdbool.h>
#include <stdint.h>

void ui_init(void);
void ui_loop(uint32_t ms_diff);

#endif // UI_H__