#include "driver/gpio.h"
#include "driver/sdmmc_defs.h"
#include "driver/sdmmc_host.h"
#include "esp32/pm.h"
#include "esp_err.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "esp_vfs_fat.h"
#include "eth_smi_iic.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "sdmmc_cmd.h"
#include "tcpip_adapter.h"
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>

extern atomic_bool pos_fix_failure;
extern uint64_t timer_diff;

volatile uint32_t frm = 0x77;

static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
	uint8_t mac_addr[6] = {1, 1};
	esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

	switch(event_id)
	{
	case ETHERNET_EVENT_CONNECTED:
		esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
		ESP_LOGI("", "Ethernet Link Up");
		ESP_LOGI("", "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
		break;
	case ETHERNET_EVENT_DISCONNECTED: ESP_LOGI("", "Ethernet Link Down"); break;
	case ETHERNET_EVENT_START: ESP_LOGI("", "Ethernet Started"); break;
	case ETHERNET_EVENT_STOP: ESP_LOGI("", "Ethernet Stopped"); break;
	default:
		break;
	}
}

static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
								 int32_t event_id, void *event_data)
{
	ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
	const tcpip_adapter_ip_info_t *ip_info = &event->ip_info;

	ESP_LOGI("", "Ethernet Got IP Address");
	ESP_LOGI("", "~~~~~~~~~~~");
	ESP_LOGI("", "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
	ESP_LOGI("", "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
	ESP_LOGI("", "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
	ESP_LOGI("", "~~~~~~~~~~~");
}

extern void imu_reset_fixture(void);
extern void lp_init(void);

int32_t PPSCount_ASM[2];
extern volatile uint16_t daq_presets[];

// volatile uint32_t gs_pnt_cnt;
// volatile uint32_t gs_pnt_cur;

#include "soc/timer_group_reg.h"

static uint8_t ram_fast[100 * 1024] = {0};
void app_main(void)
{
	esp_pm_config_esp32_t pm_config = {
		.max_freq_mhz = 240,
		.min_freq_mhz = 240,
		.light_sleep_enable = false};
	ESP_ERROR_CHECK(esp_pm_configure(&pm_config));

	gpio_pad_select_gpio(33);
	gpio_set_direction(33, GPIO_MODE_OUTPUT);

	ESP_LOGI("", "Running on core: %d\n", xPortGetCoreID());

	tcpip_adapter_init();

	ESP_ERROR_CHECK(esp_event_loop_create_default());
	ESP_ERROR_CHECK(tcpip_adapter_set_default_eth_handlers());
	ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

	eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
	mac_config.smi_mdc_gpio_num = 23;
	mac_config.smi_mdio_gpio_num = 18;
	eth_phy_config_t phy_config = {
		.phy_addr = ESP_ETH_PHY_ADDR_AUTO,
		.reset_timeout_ms = 100,
		.autonego_timeout_ms = /*4000*/ 100,
		.reset_gpio_num = -1,
	};
	phy_config.phy_addr = 0;
	vTaskDelay(pdMS_TO_TICKS(1));

	esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&mac_config);
	esp_eth_phy_t *phy = esp_eth_phy_new_lan8720(&phy_config);
	esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);

	esp_eth_handle_t eth_handle = NULL;
	ESP_ERROR_CHECK(esp_eth_driver_install(&config, &eth_handle));
	eth_smii_config(eth_handle);

	ESP_ERROR_CHECK(esp_eth_start(eth_handle));

	lp_init();

#if 1
	do
	{
		sdmmc_host_t host = SDMMC_HOST_DEFAULT();
		host.flags = SDMMC_HOST_FLAG_1BIT; // SDMMC_HOST_FLAG_DDR or SDMMC_HOST_FLAG_1BIT
		host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
		sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
		slot_config.width = 1;
		esp_vfs_fat_sdmmc_mount_config_t mount_config = {
			.format_if_mount_failed = false,
			.max_files = 5};
		sdmmc_card_t *card;
		esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
		if(ret != ESP_OK)
		{
			if(ret == ESP_FAIL)
				ESP_LOGE("SD", "Failed to mount filesystem. If you want the card to be formatted, set format_if_mount_failed = true.");
			else
				ESP_LOGE("SD", "Failed to initialize the card (%d). Make sure SD card lines have pull-up resistors in place.", ret);
			break;
		}

		sdmmc_card_print_info(stdout, card);

		ESP_LOGI("SD", "Opening file");
		FILE *f = fopen("/sdcard/hello.txt", "w");
		if(f == NULL)
		{
			ESP_LOGE("SD", "Failed to open file for writing");
			break;
		}
		fprintf(f, "Hello %s!\n", card->cid.name);
		fclose(f);
		ESP_LOGI("SD", "File written");

		struct stat st;
		if(stat("/sdcard/foo.txt", &st) == 0)
		{
			unlink("/sdcard/foo.txt"); // Delete it if it exists
		}

		ESP_LOGI("SD", "Renaming file");
		if(rename("/sdcard/hello.txt", "/sdcard/foo.txt") != 0)
		{
			ESP_LOGE("SD", "Rename failed");
			break;
		}

		ESP_LOGI("SD", "Reading file");
		f = fopen("/sdcard/foo.txt", "r");
		if(f == NULL)
		{
			ESP_LOGE("SD", "Failed to open file for reading");
			break;
		}

		char line[64];
		fgets(line, sizeof(line), f);
		fclose(f);

		char *pos = strchr(line, '\n'); // strip newline
		if(pos) *pos = '\0';
		ESP_LOGI("SD", "Read from file: '%s'", line);

		esp_vfs_fat_sdmmc_unmount(); // unmount partition and disable SDMMC host peripheral
		ESP_LOGI("SD", "Card unmounted");
	} while(0);
#endif

	while(1)
	{
		if(pos_fix_failure)
		{
			gpio_set_level(33, 1);
			vTaskDelay(200);
			imu_reset_fixture();
			gpio_set_level(33, 0);
		}
		vTaskDelay(1);

		static uint16_t cnt = 0;
		static uint16_t ccc = 0;
		if(++cnt >= 30)
		{
			// ccc++;
			cnt = 0;
			ccc++;
			// cnt = /*__builtin_bswap16*/(ccc);
			float dur_isr_us = (float)timer_diff / 40.0f;
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
