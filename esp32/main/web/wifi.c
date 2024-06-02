#include "wifi.h"
#include "esp_netif.h"
#include "lc_udp.h"
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_wifi.h>
#include <string.h>

#define WIFI_SSID "BRICK"
#define WIFI_PASS "87654321"
#define WIFI_CHNL 6
#define WIFI_MAX_STA_CONN 4

esp_netif_t *sta = 0, *ap = 0;

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
	if(event_id == WIFI_EVENT_AP_STACONNECTED)
	{
		wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
		ESP_LOGI("WIFI", "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);

		esp_netif_ip_info_t ip;
		esp_netif_get_ip_info(ap, &ip);
		lc_set_addr_self_wifi_ap(ip.ip.addr);
	}
	else if(event_id == WIFI_EVENT_AP_STADISCONNECTED)
	{
		wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
		ESP_LOGI("WIFI", "station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
	}
}

void wifi_init(void)
{
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	wifi_config_t wifi_config = {.ap = {.ssid = WIFI_SSID,
										.ssid_len = strlen(WIFI_SSID),
										.channel = WIFI_CHNL,
										.password = WIFI_PASS,
										.max_connection = WIFI_MAX_STA_CONN,
										.authmode = WIFI_AUTH_WPA_WPA2_PSK}};
	if(strlen(WIFI_PASS) == 0) wifi_config.ap.authmode = WIFI_AUTH_OPEN;

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

	ESP_LOGI("WIFI", "SSID:%s password:%s channel:%d", WIFI_SSID, WIFI_PASS, WIFI_CHNL);

	sta = esp_netif_create_default_wifi_sta();
	ap = esp_netif_create_default_wifi_ap();
	ESP_ERROR_CHECK(esp_wifi_start());
}
