#ifndef PROTO_L0_H__
#define PROTO_L0_H__

#include "proto.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct
{
	bool in_boot;
	uint8_t lpc_on;
	uint8_t pwr_on;
	uint8_t sd_detect;
	uint32_t cyc_cntr;
	uint32_t err, err_latched;
	float u_in, u_p24, u_n24, i_24;
	float t[2], t_amp, t_head, t_mcu;
	float vel_fan;
	float lum;
	float xl[3], g[3];
} stm_sts_t;

extern stm_sts_t stm_sts;

void proto_append(const volatile uint8_t *data, uint32_t len);
void proto_poll(void);

void proto_calc_fill_crc16(uint8_t *data, uint32_t len);

void proto_l1_parse(uint8_t cmd, const uint8_t *payload, uint16_t len);

void proto_make_ver_str(uint8_t fw_type, char *s);
uint32_t int_to_str(uint32_t value, uint8_t *pbuf, uint8_t len);

void proto_send_status(void);
void proto_req_params(void);

#endif // PROTO_L0_H__