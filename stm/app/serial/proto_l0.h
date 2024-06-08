#ifndef proto_L0_H__
#define proto_L0_H__

#include "proto.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct
{
	bool in_boot;
	uint8_t lpc_on;
	uint8_t pwr_on;
	uint32_t err, err_latched;
	uint32_t cyc_cntr;
} esp_sts_t;

extern esp_sts_t esp_sts;

void proto_append(const volatile uint8_t *data, uint32_t len);
void proto_poll(void);

void proto_calc_fill_crc16(uint8_t *data, uint32_t len);

void proto_l1_parse(uint8_t cmd, const uint8_t *payload, uint16_t len);

void proto_make_ver_str(uint8_t fw_type, char *s);
uint32_t int_to_str(uint32_t value, uint8_t *pbuf, uint8_t len);

void proto_send_status(void);
void proto_send_param(uint8_t param, const void *data, size_t size);
void proto_update_params(void);

#endif // proto_L0_H__