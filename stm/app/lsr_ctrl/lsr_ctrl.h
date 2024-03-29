#ifndef LSR_CTRL_H__
#define LSR_CTRL_H__

#include <stdint.h>

void lsr_ctrl_init(void);
void lsr_ctrl_init_disable(void);
void lsr_ctrl_init_enable(int x);
void lsr_ctrl_direct_cmd(uint16_t val[5]);

#endif // LSR_CTRL_H__