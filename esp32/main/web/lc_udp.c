#include "lc_udp.h"
#include "error.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lp.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "nvs_flash.h"
#include "proto_l0.h"
#include "safety.h"
#include <lwip/netdb.h>
#include <string.h>
#include <sys/param.h>

#define BC_UDP_PORT 45456
#define CMD_UDP_PORT 45457
#define FB_UDP_PORT 45458

#define PROTO_LC_PING 0x27
#define PROTO_LC_GET_STS 0x77
#define PROTO_LC_ECHO_EN 0x78
#define PROTO_LC_PWR_EN 0x80
#define PROTO_LC_FRAME_RATE 0x82
#define PROTO_LC_READ_FB_EMPTY 0x8A
#define PROTO_LC_FB_RST 0x8D
#define PROTO_LC_WR_CFG 0x97
#define PROTO_LC_FB_DATA 0xA9
#define PROTO_LC_FB_DATA_COMPRESSED 0x9A
#define PROTO_LC_ID_REQ 0xB0
#define PROTO_LC_ID_RESP 0xB1

enum
{
	STS_PWR_EN = 0,
	STS_UNLOCKED,
	STS_TEMP_WARN,
	STS_TEMP_OVER,
};

enum
{
	CONN_ETH_SRV = 1,
	CONN_WIFI_SRV,
	CONN_ETH_CLI,
	CONN_WIFI_CLI,
};

typedef struct
{
	uint16_t port;
	const char *name;
	void (*cb)(int sock, const uint8_t *data, size_t sz, struct sockaddr_storage *source_addr);
} conn_t;

typedef struct __attribute__((packed))
{
	uint8_t cmd;
	uint8_t status;
	uint8_t pl_ver;
	uint8_t fw_maj;
	uint8_t fw_min;
	uint8_t flags;
	uint8_t _0[4];
	uint32_t dacrate;
	uint32_t maxdacrate;
	uint8_t _1;
	uint16_t buf_free;
	uint16_t buf_size;
	uint8_t bat_percent;
	int8_t temp;
	uint8_t conn_type;
	char ser[6];
	char ip[4];
	uint8_t _2;
	uint8_t model_num;
	char model_name[26];
} info_t;

typedef struct __attribute__((packed))
{
	uint8_t cmd;
	uint8_t status;
	int16_t free_space;
} ring_space_t;

typedef struct __attribute__((packed))
{
	int16_t x;
	int16_t y;
	int16_t r;
	int16_t g;
	int16_t b;
} frame_t;

static info_t info = {0};
static bool enable_buf_sz_resp = false;
static uint8_t err_cnt = 0;

static int udp_send(int sock, const uint8_t *data, int len, struct sockaddr_storage *source_addr)
{
	int err = sendto(sock, data, len, 0, (struct sockaddr *)source_addr, sizeof(struct sockaddr_storage));
	if(err < 0)
	{
		ESP_LOGE("", "Error occurred during sending %d: errno %d", err, errno);
		err_cnt++;
		return err;
	}
	return 0;
}

static void cb_sock_ping(int sock, const uint8_t *msg, size_t len, struct sockaddr_storage *source_addr)
{
	if(!len) return;
	switch(msg[0])
	{
	case PROTO_LC_PING:
		udp_send(sock, (const uint8_t[]){PROTO_LC_PING, 0}, 2, source_addr);
		return;

	default: break;
	}
}

static void cb_sock_cmd(int sock, const uint8_t *msg, size_t len, struct sockaddr_storage *source_addr)
{
	if(!len) return;

	switch(msg[0])
	{
	case PROTO_LC_GET_STS:
		info.flags = (error_get() == 0 && stm_sts.err == 0 ? (1 << STS_UNLOCKED) : 0) |
					 (safety_is_pwr_enabled() ? (1 << STS_PWR_EN) : 0) |
					 (stm_sts.t_head >= 35.0f ? (1 << STS_TEMP_WARN) : 0) |
					 (stm_sts.t_head >= 40.0f ? (1 << STS_TEMP_OVER) : 0) |
					 ((err_cnt % 0x0F) << 4);
		info.buf_free = lp_get_free_buf();
		info.temp = stm_sts.t_head;
		udp_send(sock, (const uint8_t *)&info, sizeof(info), source_addr);
		return;

	case PROTO_LC_ECHO_EN:
		enable_buf_sz_resp = msg[1] ? true : false;
		return;

	case PROTO_LC_READ_FB_EMPTY:
		info.buf_free = lp_get_free_buf();
		udp_send(sock, (uint8_t[]){PROTO_LC_READ_FB_EMPTY, 0, info.buf_free & 0xFF, info.buf_free >> 8}, 4, source_addr);
		return;

	case PROTO_LC_PWR_EN:
		if(msg[1])
		{
			safety_enable_lpc();
		}
		else
		{
			safety_disable_lpc();
			lp_reset_q();
		}
		return;

	case PROTO_LC_ID_REQ:
		udp_send(sock, (uint8_t[]){PROTO_LC_ID_RESP, 0, 0}, 3, source_addr);
		return;

	case PROTO_LC_FB_RST:
		safety_disable_lpc();
		lp_reset_q();
		break;

	case PROTO_LC_FRAME_RATE:
		uint32_t rate;
		memcpy(&rate, &msg[1], 4);
		info.dacrate = rate;
		if(info.dacrate > info.maxdacrate) info.dacrate = info.maxdacrate;
		return;

	case PROTO_LC_WR_CFG:
	default: break;
	}

	ESP_LOGI("LC", "cmd %d: ", len);
	for(uint32_t i = 0; i < len; i++)
	{
		printf("x%x ", msg[i]);
	}
	printf("\n");
	fflush(stdout);
}

static void cb_sock_fb(int sock, const uint8_t *msg, size_t len, struct sockaddr_storage *source_addr)
{
	if(!len) return;
	switch(msg[0])
	{
	case PROTO_LC_FB_DATA:
		static uint32_t prev_msg = UINT32_MAX;
		int32_t cnt_points = (len - 4) / sizeof(frame_t);
		if(len < 4 + sizeof(frame_t) || ((len - 4) % sizeof(frame_t)))
		{
			err_cnt++;
			break;
		}
		uint8_t msg_num = msg[2], frm_num = msg[3];
		if(msg_num == prev_msg)
		{
			ESP_LOGE("LC", "same msg_num! msg %03d frm %03d", msg_num, frm_num);
			err_cnt++;
		}
		else
		{
			for(uint32_t i = 0; i < cnt_points; i++)
			{
				uint16_t frame[6] = {1000000 / info.dacrate};
				memcpy(&frame[1], &msg[4 + i * sizeof(frame_t)], sizeof(frame_t));
				lp_append_pointa(frame);
				if(lp_get_free_buf() == 0)
				{
					ESP_LOGE("LC", "BUF is FULL!");
					lp_trace();
					err_cnt++;
					break;
				}
			}
		}
		if(enable_buf_sz_resp)
		{
			info.buf_free = lp_get_free_buf();
			udp_send(sock, (uint8_t[]){PROTO_LC_READ_FB_EMPTY, 0, info.buf_free & 0xFF, info.buf_free >> 8}, 4, source_addr);
		}
		return;

	case PROTO_LC_FB_DATA_COMPRESSED:
	default: break;
	}

	ESP_LOGI("LC", "fb %d:", len);
	for(uint32_t i = 0; i < len; i++)
	{
		printf("x%x ", msg[i]);
	}
	printf("\n");
	fflush(stdout);
}

conn_t conn[] = {
	{BC_UDP_PORT, "LC_PING", cb_sock_ping},
	{CMD_UDP_PORT, "LC_CMD", cb_sock_cmd},
	{FB_UDP_PORT, "LC_FB", cb_sock_fb},
};

static void udp_handler_task(void *params)
{
	conn_t *s = (conn_t *)params;

	uint8_t rx_buffer[1500];
	char addr_str[128];
	struct sockaddr_in6 dest_addr;
	struct sockaddr_storage source_addr;

	while(1)
	{
		struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
		dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
		dest_addr_ip4->sin_family = AF_INET;
		dest_addr_ip4->sin_port = htons(s->port);

		int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
		if(sock < 0)
		{
			ESP_LOGE(s->name, "Unable to create socket: errno %d", errno);
			break;
		}

		struct timeval timeout;
		timeout.tv_sec = 10;
		timeout.tv_usec = 0;
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);

		int bc = 1;
		if(setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &bc, sizeof(bc)) < 0)
		{
			ESP_LOGE(s->name, "Failed to set sock options: errno %d", errno);
			closesocket(sock);
			break;
		}

		int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
		if(err < 0) ESP_LOGE(s->name, "Socket unable to bind: errno %d", errno);

		while(1)
		{
			socklen_t socklen = sizeof(source_addr);
			int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
			if(len < 0)
			{
				if(errno != 11) ESP_LOGE(s->name, "recvfrom failed: errno %d", errno);
				break;
			}
			else
			{
				if(source_addr.ss_family == PF_INET) // get the sender's ip address as string
					inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
				else if(source_addr.ss_family == PF_INET6)
					inet6_ntoa_r(((struct sockaddr_in6 *)&source_addr)->sin6_addr, addr_str, sizeof(addr_str) - 1);

				s->cb(sock, rx_buffer, len, &source_addr);
			}
		}

		if(sock != -1)
		{
			shutdown(sock, 0);
			close(sock);
		}
	}
}

void lc_udp_init(void)
{
	info.cmd = PROTO_LC_GET_STS;
	info.status = 0;
	info.pl_ver = 0;
	info.fw_maj = 0;
	info.fw_min = 21;
	info.bat_percent = 42; // x)
	info.maxdacrate = 45000;
	info.buf_size = POINT_BUF_CNT;
	info.model_num = 0;
	info.dacrate = 30000;
	info.buf_free = lp_get_free_buf();
	strcpy(info.model_name, "BRICK");

	for(uint32_t i = 0; i < 6; i++)
		info.ser[i] = esp_random() % 0xFF;

	for(uint32_t i = 0; i < sizeof(conn) / sizeof(conn[0]); i++)
	{
		xTaskCreate(udp_handler_task, conn[i].name, 4096, (void *)&conn[i], 5, NULL);
	}
}

#define addr_get_byte(ipaddr, idx) (((const uint8_t *)(&(ipaddr)))[idx])

void lc_set_addr_self_wifi_ap(uint32_t ip)
{
	info.conn_type = CONN_WIFI_SRV;
	info.ip[0] = addr_get_byte(ip, 0);
	info.ip[1] = addr_get_byte(ip, 1);
	info.ip[2] = addr_get_byte(ip, 2);
	info.ip[3] = addr_get_byte(ip, 3);
	// ESP_LOGI("LC", "CONN WIFI SRV %d.%d.%d.%d", info.ip[0], info.ip[1], info.ip[2], info.ip[3]);
}

void lc_set_addr_self_wifi_sta(uint32_t ip)
{
	info.conn_type = CONN_WIFI_CLI;
	info.ip[0] = addr_get_byte(ip, 0);
	info.ip[1] = addr_get_byte(ip, 1);
	info.ip[2] = addr_get_byte(ip, 2);
	info.ip[3] = addr_get_byte(ip, 3);
	// ESP_LOGI("LC", "CONN WIFI CLI %d.%d.%d.%d", info.ip[0], info.ip[1], info.ip[2], info.ip[3]);
}

void lc_set_addr_self_lan_srv(uint32_t ip)
{
	info.conn_type = CONN_ETH_SRV;
	info.ip[0] = addr_get_byte(ip, 0);
	info.ip[1] = addr_get_byte(ip, 1);
	info.ip[2] = addr_get_byte(ip, 2);
	info.ip[3] = addr_get_byte(ip, 3);
	// ESP_LOGI("LC", "CONN ETH SRV %d.%d.%d.%d", info.ip[0], info.ip[1], info.ip[2], info.ip[3]);
}