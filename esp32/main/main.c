#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lp.h"
#include "proto_l0.h"
#include "safety.h"
#include "serial.h"
#include "web/lan.h"
#include "web/lc_udp.h"
#include "web/web_common.h"
#include "web/wifi.h"
#include "web/ws.h"
#include <esp_ota_ops.h>

uint32_t restart_timer = 0;

void app_main(void)
{
	serial_init();

	esp_rom_gpio_pad_select_gpio(34); // sense key
	gpio_set_direction(34, GPIO_MODE_INPUT);

	esp_rom_gpio_pad_select_gpio(35); // emergency button
	gpio_set_direction(35, GPIO_MODE_INPUT);

	safety_init();
	lp_init();

	web_common_init();
	lan_init();
	wifi_init();
	ws_init();
	lc_udp_init();
	wifi_set_default_netif();

	ESP_LOGI("_", "Running on core: %d", xPortGetCoreID());
	ESP_LOGI("_", "Free heap: %d", xPortGetFreeHeapSize());

	uint32_t prev_systick = esp_log_timestamp();

	proto_req_params();
	esp_ota_mark_app_valid_cancel_rollback();
	while(1)
	{
		const uint32_t systick_now = esp_log_timestamp();
		uint32_t diff_ms = systick_now - prev_systick;
		prev_systick = systick_now;

		static uint32_t tick_ms_acc = 0;
		tick_ms_acc += diff_ms;
		if(tick_ms_acc >= 2500)
		{
			tick_ms_acc = 0;
			ws_console("ping %d\n", esp_log_timestamp());
		}

		static uint32_t tick_ms_sts_send = 0;
		tick_ms_sts_send += diff_ms;
		if(tick_ms_sts_send >= 100)
		{
			tick_ms_sts_send = 0;
			proto_send_status();
		}

		if(restart_timer)
		{
			if(restart_timer <= diff_ms)
			{
				safety_disable_power_head();
				esp_restart();
			}
			restart_timer -= diff_ms;
		}

		safety_loop(diff_ms);

		// lp_trace();
		vTaskDelay(1);
	}
}
