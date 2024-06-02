#include "safety.h"
#include "driver/gpio.h"
#include "error.h"
#include "esp_log.h"
#include "hal/gpio_ll.h"
#include "lp.h"
#include "proto_l0.h"

#define PIN_H(X) REG_WRITE(GPIO_OUT_W1TS_REG, (1 << X))
#define PIN_L(X) REG_WRITE(GPIO_OUT_W1TC_REG, (1 << X))

static uint8_t lsr_ctrl_used_by_stm = false;
static uint8_t lsr_ctrl_used_by_esp = false;

static uint8_t is_pwr_enabled = false;

void safety_init(void)
{
	esp_rom_gpio_pad_select_gpio(16);
	gpio_set_direction(16, GPIO_MODE_OUTPUT);
	PIN_H(16); // disable laser head power
}

void safety_loop(uint32_t diff_ms)
{
	safety_lsr_set_used_by_stm(stm_sts.lpc_on);

	if(error_get() == 0 && stm_sts.err == 0)
	{
		if(stm_sts.pwr_on && lsr_ctrl_used_by_stm && is_pwr_enabled == false)
		{
			safety_enable_power_head();
			ESP_LOGI("LP", "PWR EN by req.");
		}
		else if(stm_sts.pwr_on == false && lsr_ctrl_used_by_stm == false && is_pwr_enabled && lsr_ctrl_used_by_esp == false)
		{
			safety_disable_power_head();
			ESP_LOGI("LP", "PWR DIS by req.");
		}
	}
	else
	{
		if(is_pwr_enabled) ESP_LOGE("LP", "PWR DIS by ERROR");
		if(lsr_ctrl_used_by_esp) safety_disable_lpc();
		safety_disable_power_head();
	}
}

void safety_enable_power_head(void)
{
	PIN_L(16);
	is_pwr_enabled = true;
}

void safety_disable_power_head(void)
{
	PIN_H(16);
	is_pwr_enabled = false;
}

void safety_enable_lpc(void)
{
	if(error_get() || stm_sts.err) return;

	lsr_ctrl_used_by_esp = true;
	safety_enable_power_head();
	lp_on();
}

void safety_disable_lpc(void)
{
	lp_off();
	safety_disable_power_head();
	lsr_ctrl_used_by_esp = false;
}

void safety_lsr_set_used_by_stm(bool state) { lsr_ctrl_used_by_stm = state; }
uint8_t safety_lsr_is_used_by_esp(void) { return lsr_ctrl_used_by_esp; }
uint8_t safety_is_pwr_enabled(void) { return is_pwr_enabled; }