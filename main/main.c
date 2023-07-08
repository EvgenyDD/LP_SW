#include "components/fan.h"
#include "components/i2c_accel.h"
#include "components/i2c_adc.h"
#include "components/i2c_display.h"
#include "components/i2c_exp.h"
#include "components/i2c_fram.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/gpio_hal.h"
#include "hw.h"
#include "lp.h"
#include "safety.h"
#include "sd_card.h"
#include "sdkconfig.h"
#include "ui.h"
#include "web/lan.h"
#include "web/web_common.h"
#include "web/wifi.h"
#include "web/ws.h"
#include <stdatomic.h>
#include <string.h>

extern volatile uint32_t gs_pnt_cnt[1];

// duty 300 nihua 350 works
// fan 850 min @8khz
//
// 74LVCU04AD,112 NXPERIA
// 74LVC161D,112 NXPERIA

extern void lp_init(void);

extern volatile uint16_t daq_presets[];

#include "soc/timer_group_reg.h"

#if 0
	i2c_display_contrast(10);
	while(1)
	{
		i2c_display_clear();
		for(uint8_t i = 0; i < 8; i++)
		{
			// sprintf(buffer, "%d: %04d ", i, adc_get(i));
			// i2c_display_display_text(i, 0, buffer, strlen(buffer), false);
		}

		for(uint8_t i = 0; i < 3; i++)
		{
			// const char axs[] = {'X', 'Y', 'Z'};
			// sprintf(buffer, "%c: %03d ", axs[i], imu_acc[i]);
			// i2c_display_display_text(9 + i, 0, buffer, strlen(buffer), false);
		}

		// sprintf(buffer, "B: %02d ", btn);
		// i2c_display_display_text(12, 0, buffer, strlen(buffer), false);

		static uint32_t ch = 5, duty = 1000;

		// if(gpio_ll_get_level(&GPIO, 34) == 0)
		// {
		// 	daq_presets[6 * 0 + 3] = 0;
		// 	daq_presets[6 * 0 + 4] = 0;
		// 	daq_presets[6 * 0 + 5] = 0;
		// 	flagOn = 1;
		// }
		// else
		// {
		// 	if(btn)
		// 	{
		// 		if(btn_was_presed == 0)
		// 		{
		// 			btn_was_presed = 1;
		// 			if(btn == 64) //-
		// 			{
		// 				if(--ch < 3) ch = 5;
		// 				daq_presets[6 * 0 + 3] = 0;
		// 				daq_presets[6 * 0 + 4] = 0;
		// 				daq_presets[6 * 0 + 5] = 0;
		// 				duty -= 1;
		// 				// ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, duty);
		// 				// ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);

		// 				if(duty & 1)
		// 				{
		// 					lsr_volt_enable();
		// 				}
		// 				else
		// 				{
		// 					lsr_volt_disable();
		// 				}
		// 			}
		// 			else if(btn == 16) //+
		// 			{
		// 				if(++ch > 5) ch = 3;
		// 				daq_presets[6 * 0 + 3] = 0;
		// 				daq_presets[6 * 0 + 4] = 0;
		// 				daq_presets[6 * 0 + 5] = 0;
		// 				duty += 1;
		// 				// ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, duty);
		// 				// ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
		// 			}
		// 			else if(btn == 128)
		// 			{
		// 				daq_presets[6 * 0 + ch] = 1024;
		// 			}
		// 			else if(btn == 8)
		// 			{
		// 				daq_presets[6 * 0 + ch] = 2048;
		// 			}
		// 			else if(btn == 32)
		// 			{
		// 				flagOn++;
		// 				daq_presets[6 * 0 + ch] = (flagOn & 1) ? 4095 : 0;
		// 				gs_pnt_cnt[0] = (flagOn & 2) ? 2 : 1;
		// 			}
		// 		}
		// 	}
		// 	else
		// 	{
		// 		btn_was_presed = 0;
		// 	}
		// }

		sprintf(buffer, "V: %04d ", daq_presets[6 * 0 + ch]);
		i2c_display_display_text(13, 0, buffer, strlen(buffer), false);

		sprintf(buffer, "ch: %01d ", duty);
		i2c_display_display_text(14, 0, buffer, strlen(buffer), false);

		vTaskDelay(50 / portTICK_PERIOD_MS);
#if 0
			i2c_display_contrast(0xff);
			i2c_display_direction(DIRECTION180);
			// i2c_display_direction(DIRECTION180);
			i2c_display_display_text(0, 0, "M5 Stick", 8, false);
			i2c_display_display_text(1, 0, "64x128  ", 8, false);
			i2c_display_display_text(2, 0, "ABCDEFGH", 8, false);
			i2c_display_display_text(3, 0, "abcdefgh", 8, false);
			i2c_display_display_text(4, 0, "01234567", 8, false);
			i2c_display_display_text(5, 0, "Hello   ", 8, false);
			i2c_display_display_text(6, 0, "World!! ", 8, false);

			i2c_display_display_text(8, 0, "M5 Stick", 8, true);
			i2c_display_display_text(9, 0, "64x128  ", 8, true);
			i2c_display_display_text(10, 0, "ABCDEFGH", 8, true);
			i2c_display_display_text(11, 0, "abcdefgh", 8, true);
			i2c_display_display_text(12, 0, "01234567", 8, true);
			i2c_display_display_text(13, 0, "Hello   ", 8, true);
			i2c_display_display_text(14, 0, "World!! ", 8, true);

			vTaskDelay(2000 / portTICK_PERIOD_MS);

			i2c_display_clear_screen(true);
			i2c_display_direction(DIRECTION180);
			i2c_display_contrast(0xff);
			int center = 7;
			i2c_display_display_text(center, 0, "  Good  ", 8, true);
			i2c_display_display_text(center + 1, 0, "  Bye!! ", 8, true);
			vTaskDelay(2000 / portTICK_PERIOD_MS);

			// display_fadeout();

			for(int contrast = 0xff; contrast > 0; contrast = contrast - 0x10)
			{
				i2c_display_contrast(contrast);
				vTaskDelay(10);
			}
#endif
	}
#endif

void app_main(void)
{
	ESP_LOGI("", "Running on core: %d\n", xPortGetCoreID());

	hw_init();

	fan_init();
	safety_init();

	i2c_accel_init();
	i2c_adc_init();
	i2c_exp_init();

	ui_init();
#if 1
	web_common_init();
	lan_init();
	wifi_init();
	ws_init();
#endif

	sd_card_mount();

	lp_init();

	sd_card_test();

	vTaskDelay(5);

	uint8_t db = 0;
	i2c_fram_read(0, (uint8_t *)&db, 4);
	ESP_LOGI("", "FRAM fr %d", db);
	db++;
	i2c_fram_write(0, (uint8_t *)&db, 4);

	db = 0;
	i2c_fram_read(2 * 1024 - 1, (uint8_t *)&db, 1);
	db++;
	ESP_LOGI("", "FRAM lt %d", db);
	i2c_fram_write(2 * 1024 - 1, (uint8_t *)&db, 1);

	// lsr_volt_disable();

	ESP_LOGI("HEAP", "Free heap: %d", xPortGetFreeHeapSize());

	uint32_t prev_systick = esp_log_timestamp();
	while(1)
	{
		const uint32_t systick_now = esp_log_timestamp();
		uint32_t diff_ms = systick_now - prev_systick;
		prev_systick = systick_now;

		safety_loop();

		i2c_adc_read();
		buttons_read(1); // todo debounce

		ui_loop(0, diff_ms);

		{
			// do
			// {
			// 	i2c_accel_get_data(imu_acc);
			// } while(imu_acc[0] == 255 || imu_acc[1] == 255 || imu_acc[2] == 255);

			// if(!pos_fix_failure && (imu_acc[0] || imu_acc[1] || imu_acc[2]))
			// {
			// 	if(abs((int)imu_acc[0] - (int)imu_acc_fix[0]) > ACCEL_THRS ||
			// 	   abs((int)imu_acc[1] - (int)imu_acc_fix[1]) > ACCEL_THRS ||
			// 	   abs((int)imu_acc[2] - (int)imu_acc_fix[2]) > ACCEL_THRS)
			// 	{
			// 		pos_fix_failure = true;
			// 		ESP_LOGE("", "3D pos fixture failure! %d %d %d", imu_acc[0], imu_acc[1], imu_acc[2]);
			// 	}
			// }
		}

		// static uint32_t fan_spd = 800;
		// if(btn_j_l.pressed_shot)
		// {
		// 	fan_spd -= 10;
		// 	fan_set_level(fan_spd);
		// }
		// else if(btn_j_r.pressed_shot)
		// {
		// 	fan_spd += 10;
		// 	fan_set_level(fan_spd);
		// }

		// static uint8_t colr = CLR_R;
		// if(btn_j_l.pressed_shot)
		// {
		// 	if(--colr < CLR_NULL) colr = CLR_WHT;
		// }
		// else if(btn_j_r.pressed_shot)
		// {
		// 	if(++colr > CLR_WHT) colr = CLR_NULL;
		// }

		// static bool xc = false;
		// if(btn_j_press.pressed_shot)
		// {
		// 	xc = true;
		// 	// lp_square(colr);
		// }
		// if(btn_j_press.unpressed_shot)
		// {
		// 	xc = false;
		// 	lp_blank();
		// }

		// if(pos_fix_failure)
		// {
		// 	vTaskDelay(200);
		// 	accel_reset_fixture();
		// }

		vTaskDelay(1);

		// if(gpio_ll_get_level(&GPIO, 34) == 0)
		// {
		// 	static uint8_t iter = 0;
		// 	// if(++iter >= 3) iter = 0;
		// 	// daq_presets[6 * 0 + 1] = iter == 0 ? 0 : iter == 1 ? (2047)
		// 	// 												   : (4095); // X
		// 	// daq_presets[6 * 0 + 2] = iter == 0 ? 0 : iter == 1 ? (2047)
		// 	// 												   : (4095); // Y
		// 	if(++iter & 1)
		// 		gs_pnt_cnt[0] = 1;
		// 	else
		// 		gs_pnt_cnt[0] = 4;
		// 	ESP_LOGI("===", "Now value is %d\n", daq_presets[6 * 0 + 1]);
		// 	while(gpio_ll_get_level(&GPIO, 34) == 0)
		// 		vTaskDelay(2);
		// }
#if 0
	/* Make pads GPIO */
	gpio_pad_select_gpio(OLIMEX_LED_PIN);
	gpio_pad_select_gpio(OLIMEX_BUT_PIN);

	/* Set the Relay as a push/pull output */
	gpio_set_direction(OLIMEX_LED_PIN, GPIO_MODE_OUTPUT);

	/* Set Button as input */
	gpio_set_direction(OLIMEX_BUT_PIN, GPIO_MODE_INPUT);
	/* Enable interrupt on both edges */
	gpio_set_intr_type(OLIMEX_BUT_PIN, GPIO_INTR_LOW_LEVEL);

	/* Install ISR routine */
	gpio_install_isr_service(ESP_INTR_FLAG_LOWMED);
	gpio_isr_handler_add(OLIMEX_BUT_PIN, gpio_isr_handler, 0);
}
#endif

		static uint16_t cnt = 0;
		static uint16_t ccc = 0;
		if(++cnt >= 30)
		{
			// ccc++;
			cnt = 0;
			ccc++;
			// cnt = /*__builtin_bswap16*/(ccc);
			// float dur_isr_us = (float)timer_diff / 40.0f;
			// ESP_LOGI("---", "Load: %.3f us | Load: %.1f %% | Freq: %d Hz | %d %d", dur_isr_us, 100.0f * dur_isr_us / 22.0f, 1000000 / 22, PPSCount_ASM[0], PPSCount_ASM[1]);

			// daq_presets[0] = (ccc & 1) ? 0 : 4095;
			// daq_presets[1] = (ccc & 1) ? 0 : 4095;
			// daq_presets[2] = (ccc & 1) ? 0 : 4095;
			// daq_presets[3] = (ccc & 1) ? 0 : 4095;
			// daq_presets[4] = (ccc & 1) ? 0 : 4095;
		}

		// volatile uint16_t v0 = 0x1234, v1=cnt;
		// asm("nop");
		// asm("nop");
		// gs_pnt_cur++;
		// if(gs_pnt_cur >= gs_pnt_cnt) gs_pnt_cur = 0;
		// gs_pnt_cur*=5;
		// SPI1.data_buf[0] = 176 | __builtin_bswap16(daq_presets[3 + gs_pnt_cur]);
		// asm("nop");
		// SPI1.data_buf[0] = /*(1 << (15 - 8)) | (1 << (13 - 8)) | (1 << (12 - 8)) |*/ /*__builtin_bswap16*/ (daq_presets[4 + frm]);
		// asm("nop");
		// SPI1.data_buf[0] = /*(1 << (15 - 8)) | (1 << (13 - 8)) | (1 << (12 - 8)) |*/ /*__builtin_bswap16*/ (daq_presets[5 + frm]);
		// asm("nop");
		// asm("nop");
		// asm("nop");
		// printf("%d", v1);
	}
}
