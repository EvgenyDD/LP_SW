#include "adc.h"
#include "buttons.h"
#include "config_system.h"
#include "crc.h"
#include "error.h"
#include "fan.h"
#include "fram.h"
#include "fw_header.h"
#include "i2c_common.h"
#include "i2c_display.h"
#include "lsr_ctrl.h"
#include "platform.h"
#include "prof.h"
#include "ret_mem.h"
#include "usbd_proto_core.h"
#include <stdio.h>
#include <string.h>

int gsts = -10;

#define SYSTICK_IN_US (168000000 / 1000000)
#define SYSTICK_IN_MS (168000000 / 1000)

bool g_stay_in_boot = false;

uint32_t g_uid[3];

volatile uint64_t system_time = 0;
static int32_t prev_systick = 0;

uint8_t tmp = 0;
config_entry_t g_device_config[] = {
	{"0", 1, 0, &tmp},
};
const uint32_t g_device_config_count = sizeof(g_device_config) / sizeof(g_device_config[0]);

void delay_ms(volatile uint32_t delay_ms)
{
	volatile uint32_t start = 0;
	int32_t mark_prev = 0;
	prof_mark(&mark_prev);
	const uint32_t time_limit = delay_ms * SYSTICK_IN_MS;
	for(;;)
	{
		start += (uint32_t)prof_mark(&mark_prev);
		if(start >= time_limit)
			return;
	}
}

static void end_loop(void)
{
	delay_ms(2000);
	platform_reset();
}

uint8_t addr_acked[128] = {0};
uint32_t addr_count = 0;
static char buffer[128];
__attribute__((noreturn)) void main(void)
{
	RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;

	platform_get_uid(g_uid);

	prof_init();
	platform_watchdog_init();

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA |
							   RCC_AHB1Periph_GPIOB |
							   RCC_AHB1Periph_GPIOC |
							   RCC_AHB1Periph_GPIOD,
						   ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_7; // DCDC_FAULT, SNS_KEY
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	fw_header_check_all();

	ret_mem_init();
	ret_mem_set_load_src(LOAD_SRC_APP); // let preboot know it was booted from bootloader

	adc_init();
	i2c_init();
	for(uint32_t i = 0; i < 127; i++)
	{
		uint8_t data = 0;
		if(i2c_read(i, &data, 1, false) == 0)
		{
			addr_acked[addr_count++] = i;
		}
	}

	usb_init();

	fan_init();

	lsr_ctrl_init();
	lsr_ctrl_init_enable(0);

	i2c_display_init();
	i2c_display_direction(DIRECTION0);
	i2c_display_clear_screen(0);

	fram_init();
	int sts_fram = fram_check_sn();
	if(sts_fram)
	{
	}
	fram_data_read();

	buttons_init();

	if(config_validate() == CONFIG_STS_OK) config_read_storage();

	for(;;)
	{
		// time diff
		uint32_t time_diff_systick = (uint32_t)prof_mark(&prev_systick);

		static uint32_t remain_systick_us_prev = 0, remain_systick_ms_prev = 0;
		uint32_t diff_us = (time_diff_systick + remain_systick_us_prev) / (SYSTICK_IN_US);
		remain_systick_us_prev = (time_diff_systick + remain_systick_us_prev) % SYSTICK_IN_US;

		uint32_t diff_ms = (time_diff_systick + remain_systick_ms_prev) / (SYSTICK_IN_MS);
		remain_systick_ms_prev = (time_diff_systick + remain_systick_ms_prev) % SYSTICK_IN_MS;

		platform_watchdog_reset();

		system_time += diff_ms;

		adc_track();
		buttons_poll(diff_ms);

		static uint8_t upd_cnt, upd_cnt2;
		upd_cnt += diff_ms;
		upd_cnt2 += diff_ms;

		usb_poll(diff_ms);

		static uint32_t v = 50;
		if(btn_act[0].state == BTN_PRESS_SHOT)
		{
			v += 50;
			uint16_t raw[] = {0, 0, 0, 0, 0};
			lsr_ctrl_direct_cmd(raw);
		}
		else if(btn_act[2].state == BTN_PRESS_SHOT)
		{
			v -= 50;
			uint16_t raw[] = {4095, 4095, 0, 0, 0};
			lsr_ctrl_direct_cmd(raw);
		}
		if(v > 1023) v = 1023;

		if(upd_cnt2 >= 10)
		{
			upd_cnt2 = 0;
			static uint32_t ph = 0;
			if(++ph >= 4) ph = 0;
			if(ph == 0)
			{
				uint16_t raw[] = {2048 + v, 2048 + v * 2, 0, 100, 0};
				lsr_ctrl_direct_cmd(raw);
			}
			else if(ph == 1)
			{
				uint16_t raw[] = {2048 - v, 2048 + v * 2, 0, 100, 0};
				lsr_ctrl_direct_cmd(raw);
			}
			else if(ph == 2)
			{
				uint16_t raw[] = {2048 - v, 2048 - v * 2, 0, 100, 0};
				lsr_ctrl_direct_cmd(raw);
			}
			else if(ph == 3)
			{
				uint16_t raw[] = {2048 + v, 2048 - v * 2, 0, 100, 0};
				lsr_ctrl_direct_cmd(raw);
			}
		}
		if(upd_cnt >= 100)
		{
			upd_cnt = 0;

			// static uint8_t upd_cnt2;
			// if(++upd_cnt2 >= 25)
			// {
			// 	upd_cnt2 = 0;
			// 	static uint8_t statee = 0;
			// 	statee++;
			// 	lsr_ctrl_init_enable(statee & 1);
			// }

			static uint16_t cntr = 0;
			sprintf(buffer, "%.2fA", adc_val.i24);
			i2c_display_display_text(0, 0, buffer, strlen(buffer), false);

			sprintf(buffer, "%.1fV", adc_val.vp24);
			i2c_display_display_text(1, 0, buffer, strlen(buffer), false);

			sprintf(buffer, "%.1fV", adc_val.vm24);
			i2c_display_display_text(2, 0, buffer, strlen(buffer), false);

			sprintf(buffer, "%.1fV", adc_val.vin);
			i2c_display_display_text(3, 0, buffer, strlen(buffer), false);

			sprintf(buffer, "%x", cntr++);
			i2c_display_display_text(4, 0, buffer, strlen(buffer), false);

			sprintf(buffer, "V:%ld ", v);
			i2c_display_display_text(5, 0, buffer, strlen(buffer), false);
		}
	}
}