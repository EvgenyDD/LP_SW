#include "fan.h"
#include "driver/ledc.h"

#define FAN_MIN_PWM 850

static uint16_t fan_value = 0;
static ledc_channel_config_t ledc_channel;

void fan_init(void)
{
	ledc_timer_config_t ledc_timer = {
		.duty_resolution = LEDC_TIMER_10_BIT,
		.freq_hz = 8000,
		.speed_mode = LEDC_HIGH_SPEED_MODE,
		.timer_num = LEDC_TIMER_0,
		.clk_cfg = LEDC_AUTO_CLK,
	};

	ledc_timer_config(&ledc_timer);
	ledc_channel.channel = LEDC_CHANNEL_0;
	ledc_channel.duty = 0;
	ledc_channel.gpio_num = 16;
	ledc_channel.speed_mode = LEDC_HIGH_SPEED_MODE;
	ledc_channel.hpoint = 0;
	ledc_channel.timer_sel = LEDC_TIMER_0;
	ledc_channel_config(&ledc_channel);

	ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 0);
	ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
}

void fan_set_level(uint8_t value)
{
	fan_value = FAN_MIN_PWM + value;
	if(fan_value > 1023) fan_value = 1023;
	ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, fan_value);
	ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
}

uint16_t fan_get_level(void) { return fan_value; }