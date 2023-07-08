#include "lan.h"
#include "esp_err.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "lwip/netif.h"

#define TAG "ETH"

void lan_set_default_netif(void);
void wifi_set_default_netif(void);

static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
	uint8_t mac_addr[6] = {1, 1};
	esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

	switch(event_id)
	{
	case ETHERNET_EVENT_CONNECTED:
		esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
		ESP_LOGI(TAG, "Ethernet Link Up | MAC %02x:%02x:%02x:%02x:%02x:%02x", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
		lan_set_default_netif();
		break;

	case ETHERNET_EVENT_DISCONNECTED:
		ESP_LOGI(TAG, "Ethernet Link Down");
		wifi_set_default_netif();
		break;

	case ETHERNET_EVENT_START: ESP_LOGI(TAG, "Ethernet Started"); break;
	case ETHERNET_EVENT_STOP: ESP_LOGI(TAG, "Ethernet Stopped"); break;
	default: break;
	}
}

static void got_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
	const ip_event_got_ip_t *event = (const ip_event_got_ip_t *)event_data;
	ESP_LOGI(TAG, "Got IP: " IPSTR ", mask: " IPSTR ", gw: " IPSTR, IP2STR(&event->ip_info.ip), IP2STR(&event->ip_info.netmask), IP2STR(&event->ip_info.gw));
}

void lan_init(void)
{
	// tcpip_adapter_init();

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

	const esp_netif_ip_info_t my_ap_ip = {
		.ip = {.addr = PP_HTONL(LWIP_MAKEU32(192, 168, 1, 1))},
		.gw = {.addr = PP_HTONL(LWIP_MAKEU32(192, 168, 1, 1))},
		.netmask = {.addr = PP_HTONL(LWIP_MAKEU32(255, 255, 255, 0))},
	};

	const esp_netif_inherent_config_t eth_behav_cfg = {
		.get_ip_event = IP_EVENT_ETH_GOT_IP,
		.lost_ip_event = 0,
		.flags = ESP_NETIF_DHCP_SERVER | ESP_NETIF_FLAG_AUTOUP,
		.ip_info = (esp_netif_ip_info_t *)&my_ap_ip,
		.if_key = "ETHDHCPS",
		.if_desc = "eth",
		.route_prio = 50};

	esp_netif_config_t my_eth_cfg = {.base = &eth_behav_cfg, .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH};

	esp_netif_t *eth_netif = esp_netif_new(&my_eth_cfg);

	esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&mac_config);
	esp_eth_phy_t *phy = esp_eth_phy_new_lan8720(&phy_config);
	esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
	esp_eth_handle_t eth_handle = NULL;

	ESP_ERROR_CHECK(esp_eth_driver_install(&config, &eth_handle));
	ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));
	ESP_ERROR_CHECK(esp_eth_start(eth_handle));
}

void lan_set_default_netif(void)
{
	for(struct netif *pri = netif_list; pri != NULL; pri = pri->next)
	{
		if(pri->name[0] == 'e' &&
		   pri->name[1] == 'n')
		{
			ESP_LOGI(TAG, "Set default NETIF %c%c%d (" IPSTR "/" IPSTR " gateway " IPSTR ")",
					 pri->name[0],
					 pri->name[1],
					 pri->num,
					 IP2STR(&pri->ip_addr.u_addr.ip4),
					 IP2STR(&pri->netmask.u_addr.ip4),
					 IP2STR(&pri->gw.u_addr.ip4));
			netif_set_default(pri);
			break;
		}
	}
	esp_wifi_stop();
}

void wifi_set_default_netif(void)
{
	esp_wifi_start();
	for(struct netif *pri = netif_list; pri != NULL; pri = pri->next)
	{
		if(pri->name[0] == 'a' &&
		   pri->name[1] == 'p')
		{
			ESP_LOGI(TAG, "Set default NETIF %c%c%d (" IPSTR "/" IPSTR " gateway " IPSTR ")",
					 pri->name[0],
					 pri->name[1],
					 pri->num,
					 IP2STR(&pri->ip_addr.u_addr.ip4),
					 IP2STR(&pri->netmask.u_addr.ip4),
					 IP2STR(&pri->gw.u_addr.ip4));
			netif_set_default(pri);
			break;
		}
	}
}