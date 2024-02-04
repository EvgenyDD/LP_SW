#include "safety.h"

extern float map(float x, float in_min, float in_max, float out_min, float out_max);

#define TIMER_VAL_RESET_ERR 500

static bool safety_lock = true;
static uint32_t reset_timer = 0;

void safety_init(void)
{
	safety_lock = true;
	// lsr_volt_enable();
}

void safety_reset_lock(void)
{
	if(safety_check_error()) return;
	reset_timer = TIMER_VAL_RESET_ERR;
	// lsr_volt_enable();
}

bool safety_is_locked(void)
{
	return safety_lock;
}