#ifndef LC_UDP_H__
#define LC_UDP_H__

#include "esp_netif.h"

void lc_udp_init(void);

void lc_set_addr_self_wifi_ap(uint32_t ip);
void lc_set_addr_self_wifi_sta(uint32_t ip);
void lc_set_addr_self_lan_srv(uint32_t ip);

#endif // LC_UDP_H__