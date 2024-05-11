// #include "../../common/proto.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hw.h"
#include "lp.h"
#include "safety.h"
#include "web/lan.h"
#include "web/web_common.h"
#include "web/wifi.h"
#include "web/ws.h"

void app_main(void)
{
	ESP_LOGI("", "Running on core: %d\n", xPortGetCoreID());

	hw_init();

	safety_init();

#if 1
	web_common_init();
	lan_init();
	wifi_init();
	ws_init();
#endif

	lp_init();

	ESP_LOGI("HEAP", "Free heap: %d", xPortGetFreeHeapSize());

	uint32_t prev_systick = esp_log_timestamp();

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

		safety_loop(diff_ms);

		vTaskDelay(1);
	}
}
