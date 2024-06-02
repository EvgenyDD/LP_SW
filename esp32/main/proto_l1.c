#include "error.h"
#include "esp_log.h"
#include "lp.h"
#include "proto.h"
#include "proto_l0.h"
#include "safety.h"
#include "serial.h"
#include <stdio.h>
#include <string.h>

stm_sts_t stm_sts;

static uint8_t pkt_buf[512];

uint32_t int_to_str(uint32_t value, uint8_t *pbuf, uint8_t len)
{
	for(uint8_t idx = 0; idx < len; idx++, value <<= 4)
		pbuf[idx] = (value >> 28) + ((value >> 28) < 0xA ? '0' : ('A' - 10));
	return len;
}

static void sts_fill(uint8_t *_pkt, uint32_t *ptr, uint32_t key, uint32_t len, const void *data)
{
	_pkt[(*ptr)++] = key;
	if(!len) return;
	memcpy(&_pkt[*ptr], data, len);
	*ptr += len;
}

static void send_b1(uint8_t cmd, uint8_t code)
{
	uint32_t ptr = 2;
	pkt_buf[ptr++] = cmd;
	pkt_buf[ptr++] = code;
	memcpy(&pkt_buf[0], (uint16_t[]){ptr + 2}, 2);
	proto_calc_fill_crc16(pkt_buf, ptr);

	serial_tx(pkt_buf, ptr + 2);
}

void proto_send_status(void)
{
	uint32_t ptr = 2;
	pkt_buf[ptr++] = PROTO_CMD_STS;
	pkt_buf[ptr++] = PROTO_STS_SW_TYPE_FW;

	uint32_t err = error_get();
	sts_fill(pkt_buf, &ptr, PROTO_STS_ERR, sizeof(uint32_t), &err);
	err = error_get_latched();
	sts_fill(pkt_buf, &ptr, PROTO_STS_ERR_LATCHED, sizeof(uint32_t), &err);

	static uint32_t incr_cnt = 0;
	sts_fill(pkt_buf, &ptr, PROTO_STS_INCR_CNT, sizeof(incr_cnt), &incr_cnt);
	incr_cnt++;

	uint8_t lp_on = safety_lsr_is_used_by_esp();
	sts_fill(pkt_buf, &ptr, PROTO_STS_LPC_ACT, sizeof(lp_on), &lp_on);

	uint8_t pwr_on = safety_is_pwr_enabled();
	sts_fill(pkt_buf, &ptr, PROTO_STS_GALV_LSR_PWR_EN, sizeof(pwr_on), &pwr_on);

	memcpy(&pkt_buf[0], (uint16_t[]){ptr + 2}, 2);
	proto_calc_fill_crc16(pkt_buf, ptr);

	serial_tx(pkt_buf, ptr + 2);
}

static uint32_t get_uint32(const uint8_t *p)
{
	uint32_t v;
	memcpy(&v, p, sizeof(uint32_t));
	return v;
}

// static int16_t get_int16(const uint8_t *p)
// {
// 	int16_t v;
// 	memcpy(&v, p, sizeof(uint16_t));
// 	return v;
// }

// static uint16_t get_uint16(const uint8_t *p)
// {
// 	uint16_t v;
// 	memcpy(&v, p, sizeof(uint16_t));
// 	return v;
// }

static float get_float(const uint8_t *p)
{
	float v;
	memcpy(&v, p, sizeof(float));
	return v;
}

static bool parse_status(const uint8_t *pl, uint32_t size)
{
	for(uint32_t ptr = 0;;)
	{
		if(ptr > size) return 1; // error case
		if(ptr == size) return 0;

		switch(pl[ptr])
		{
		case PROTO_STS_SW_TYPE_BOOT:
			stm_sts.in_boot = true;
			ptr += 1 + 0;
			break;
		case PROTO_STS_SW_TYPE_FW:
			stm_sts.in_boot = false;
			ptr += 1 + 0;
			break;

		case PROTO_STS_ERR:
			stm_sts.err = get_uint32(&pl[1 + ptr]);
			ptr += 1 + sizeof(uint32_t);
			break;

		case PROTO_STS_ERR_LATCHED:
			stm_sts.err_latched = get_uint32(&pl[1 + ptr]);
			ptr += 1 + sizeof(uint32_t);
			break;

		case PROTO_STS_SD_CARD_INSRT:
			if(pl[1 + ptr] != stm_sts.sd_detect) ESP_LOGI("SD Card", "%d", pl[1 + ptr]);
			stm_sts.sd_detect = pl[1 + ptr];
			ptr += 1 + sizeof(uint8_t);
			break;

		case PROTO_STS_LPC_ACT:
			stm_sts.lpc_on = pl[1 + ptr];
			ptr += 1 + sizeof(uint8_t);
			break;

		case PROTO_STS_GALV_LSR_PWR_EN:
			stm_sts.pwr_on = pl[1 + ptr];
			ptr += 1 + sizeof(uint8_t);
			break;

		case PROTO_STS_U_IN:
			stm_sts.u_in = get_float(&pl[1 + ptr]);
			ptr += 1 + sizeof(float);
			break;

		case PROTO_STS_U_P24:
			stm_sts.u_p24 = get_float(&pl[1 + ptr]);
			ptr += 1 + sizeof(float);
			break;

		case PROTO_STS_U_N24:
			stm_sts.u_n24 = get_float(&pl[1 + ptr]);
			ptr += 1 + sizeof(float);
			break;

		case PROTO_STS_I_24:
			stm_sts.i_24 = get_float(&pl[1 + ptr]);
			ptr += 1 + sizeof(float);
			break;

		case PROTO_STS_T_INV0:
			stm_sts.t[0] = get_float(&pl[1 + ptr]);
			ptr += 1 + sizeof(float);
			break;

		case PROTO_STS_T_INV1:
			stm_sts.t[1] = get_float(&pl[1 + ptr]);
			ptr += 1 + sizeof(float);
			break;

		case PROTO_STS_T_AMP:
			stm_sts.t_amp = get_float(&pl[1 + ptr]);
			ptr += 1 + sizeof(float);
			break;

		case PROTO_STS_T_HEAD:
			stm_sts.t_head = get_float(&pl[1 + ptr]);
			ptr += 1 + sizeof(float);
			break;

		case PROTO_STS_T_MCU:
			stm_sts.t_mcu = get_float(&pl[1 + ptr]);
			ptr += 1 + sizeof(float);
			break;

		case PROTO_STS_VEL_FAN:
			stm_sts.vel_fan = get_float(&pl[1 + ptr]);
			ptr += 1 + sizeof(float);
			break;

		case PROTO_STS_LUM:
			stm_sts.lum = get_float(&pl[1 + ptr]);
			ptr += 1 + sizeof(float);
			break;

		case PROTO_STS_XL_X:
		case PROTO_STS_XL_Y:
		case PROTO_STS_XL_Z:
			stm_sts.xl[pl[ptr] - PROTO_STS_XL_X] = get_float(&pl[1 + ptr]);
			ptr += 1 + sizeof(float);
			break;

		case PROTO_STS_G_X:
		case PROTO_STS_G_Y:
		case PROTO_STS_G_Z:
			stm_sts.g[pl[ptr] - PROTO_STS_G_X] = get_float(&pl[1 + ptr]);
			ptr += 1 + sizeof(float);
			break;

		case PROTO_STS_INCR_CNT:
			stm_sts.cyc_cntr = get_uint32(&pl[1 + ptr]);
			ptr += 1 + sizeof(uint32_t);
			break;

		default: return 1; // error case
		}
	}
}

void proto_l1_parse(uint8_t cmd, const uint8_t *payload, uint16_t len)
{
	switch(cmd)
	{
	case PROTO_CMD_STS:
		error_set(ERR_ESP_PROTO, parse_status(payload, len));
		break;

	default: send_b1(cmd, 1); break;
	}
}
