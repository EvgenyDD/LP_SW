#include "../../common/proto.h"
#include "adc.h"
#include "buttons.h"
#include "config_system.h"
#include "crc.h"
#include "error.h"
#include "fram.h"
#include "fw_header.h"
#include "i2c_common.h"
#include "i2c_display.h"
#include "imu.h"
#include "lsr_ctrl.h"
#include "opt3001.h"
#include "platform.h"
#include "prof.h"
#include "ret_mem.h"
#include "safety.h"
#include "serial.h"
#include "ui.h"
#include "usbd_proto_core.h"
#include <stdio.h>
#include <string.h>

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

static char buffer[128];

__attribute__((noreturn)) void main(void)
{
	RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;

	platform_get_uid(g_uid);

	prof_init();
	platform_watchdog_init();

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD, ENABLE);

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

	imu_init();
	opt3001_init();

	serial_init();

	usb_init();

	fan_init();

	lsr_ctrl_init();
	i2c_display_init();
	i2c_display_direction(DIRECTION0);
	i2c_display_clear_screen(0);

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

		safety_loop(diff_ms);
		usb_poll(diff_ms);
		ui_loop(diff_ms);
		fan_track(diff_ms);

		error_set(ERROR_KEY, GPIOC->IDR & (1 << 7));
		error_set(ERROR_FAN, fan_get_vel() == 0);
		error_set(ERROR_PS, !(GPIOC->IDR & (1 << 3)));
		error_set(ERROR_RB, (GPIOA->IDR & (1 << 8)));

		// (ERROR_SFTY)
		// (ERROR_CFG)
		// (ERROR_FRAM)
		// (ERROR_OT)
	}
}