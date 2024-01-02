#include "safety.h"
#include "fan.h"
#include "i2c_adc.h"
#include "i2c_exp.h"

extern float map(float x, float in_min, float in_max, float out_min, float out_max);

#define TEMP_THRS 32
#define TEMP_THRS_CRITICAL 60
#define TIMER_VAL_RESET_ERR 500

static bool safety_lock = true;
static uint32_t reset_timer = 0;

void safety_init(void)
{
	safety_lock = true;
	lsr_volt_enable();
}

bool safety_check_error(void)
{
	return (adc_val.t_drv / 10 > TEMP_THRS_CRITICAL ||
			adc_val.t_lsr_head / 10 > TEMP_THRS_CRITICAL ||
			adc_val.t_inv_p / 10 > TEMP_THRS_CRITICAL ||
			adc_val.t_inv_n / 10 > TEMP_THRS_CRITICAL);
}

void safety_loop(uint32_t diff_ms)
{
	float maxt = adc_val.t_drv / 10;
	if(adc_val.t_lsr_head / 10 > maxt) maxt = adc_val.t_lsr_head / 10;
	if(adc_val.t_inv_p / 10 > maxt) maxt = adc_val.t_inv_p / 10;
	if(adc_val.t_inv_n / 10 > maxt) maxt = adc_val.t_inv_n / 10;
	if(maxt > TEMP_THRS)
	{
		fan_set_level(map(maxt, TEMP_THRS, TEMP_THRS_CRITICAL, 10, 150));
	}
	else
	{
		fan_set_level(0);
	}

	if(reset_timer)
	{
		reset_timer = reset_timer >= diff_ms ? reset_timer - diff_ms : 0;
		if(btn_emcy.pressed == false)
		{
			safety_lock = false;
		}
	}
	else
	{
		if(btn_emcy.pressed)
		{
			safety_lock = true;
		}

		if(safety_lock)
		{
			lsr_volt_disable();
		}
	}
}

void safety_reset_lock(void)
{
	if(safety_check_error()) return;
	reset_timer = TIMER_VAL_RESET_ERR;
	lsr_volt_enable();
}

bool safety_is_locked(void)
{
	return safety_lock;
}