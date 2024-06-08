#include "adc.h"
#include "buttons.h"
#include "config_system.h"
#include "error.h"
#include "fram.h"
#include "fw_header.h"
#include "imu.h"
#include "lsr_ctrl.h"
#include "opt3001.h"
#include "platform.h"
#include "proto.h"
#include "proto_l0.h"
#include "ret_mem.h"
#include "safety.h"
#include "serial.h"
#include <string.h>

esp_sts_t esp_sts;

static uint8_t pkt_buf[512];

uint32_t int_to_str(uint32_t value, uint8_t *pbuf, uint8_t len)
{
	for(uint8_t idx = 0; idx < len; idx++, value <<= 4)
		pbuf[idx] = (value >> 28) + ((value >> 28) < 0xA ? '0' : ('A' - 10));
	return len;
}

void proto_make_ver_str(uint8_t fw_type, char *s)
{
	uint32_t ptr = 0;

	uint32_t num = g_fw_info[fw_type].ver_patch;
	do
	{
		s[ptr++] = num % 10 + '0';
		num /= 10;
	} while(num);

	s[ptr++] = '.';

	num = g_fw_info[fw_type].ver_minor;
	do
	{
		s[ptr++] = num % 10 + '0';
		num /= 10;
	} while(num);

	s[ptr++] = '.';

	num = g_fw_info[fw_type].ver_major;
	do
	{
		s[ptr++] = num % 10 + '0';
		num /= 10;
	} while(num);

	for(uint32_t i = 0, j = ptr - 1; i < j; i++, j--)
	{
		uint8_t a = s[i];
		s[i] = s[j];
		s[j] = a;
	}
	s[ptr++] = '\0';
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

void proto_send_param(uint8_t param, const void *data, size_t size)
{
	uint32_t ptr = 2;
	pkt_buf[ptr++] = PROTO_CMD_PARAM_SET;
	pkt_buf[ptr++] = param;
	memcpy(&pkt_buf[ptr], data, size);
	ptr += size;
	memcpy(&pkt_buf[0], (uint16_t[]){ptr + 2}, 2);
	proto_calc_fill_crc16(pkt_buf, ptr);
	serial_tx(pkt_buf, ptr + 2);
}

static void send_str(uint8_t cmd, const uint8_t *s)
{
	uint16_t slen;
	for(slen = 0; s[slen] != '\0' && slen < PROTO_MAX_PKT_SIZE - 6; slen++)
		;
	uint16_t ptr = 2;
	pkt_buf[ptr++] = cmd;
	memcpy(&pkt_buf[ptr], s, slen);
	ptr += slen;
	pkt_buf[ptr++] = '\0';
	uint16_t len = ptr + 2L;
	pkt_buf[2] = len & 0xFF;
	pkt_buf[3] = (len >> 8L) & 0xFF;
	proto_calc_fill_crc16(pkt_buf, ptr);
	serial_tx(pkt_buf, len);
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

	sts_fill(pkt_buf, &ptr, PROTO_STS_I_24, sizeof(float), &adc_val.i24);
	sts_fill(pkt_buf, &ptr, PROTO_STS_U_IN, sizeof(float), &adc_val.vin);
	sts_fill(pkt_buf, &ptr, PROTO_STS_U_P24, sizeof(float), &adc_val.vp24);
	sts_fill(pkt_buf, &ptr, PROTO_STS_U_N24, sizeof(float), &adc_val.vn24);
	sts_fill(pkt_buf, &ptr, PROTO_STS_T_MCU, sizeof(float), &adc_val.t_mcu);
	sts_fill(pkt_buf, &ptr, PROTO_STS_T_INV0, sizeof(float), &adc_val.t[0]);
	sts_fill(pkt_buf, &ptr, PROTO_STS_T_INV1, sizeof(float), &adc_val.t[1]);
	sts_fill(pkt_buf, &ptr, PROTO_STS_T_AMP, sizeof(float), &adc_val.t_amp);
	sts_fill(pkt_buf, &ptr, PROTO_STS_T_HEAD, sizeof(float), &adc_val.t_head);

	float fan = fan_get_vel();
	sts_fill(pkt_buf, &ptr, PROTO_STS_VEL_FAN, sizeof(float), &fan);
	sts_fill(pkt_buf, &ptr, PROTO_STS_LUM, sizeof(float), &opt3001_lux);

	sts_fill(pkt_buf, &ptr, PROTO_STS_XL_X, sizeof(float), &imu_val.xl[0]);
	sts_fill(pkt_buf, &ptr, PROTO_STS_XL_Y, sizeof(float), &imu_val.xl[1]);
	sts_fill(pkt_buf, &ptr, PROTO_STS_XL_Z, sizeof(float), &imu_val.xl[2]);
	sts_fill(pkt_buf, &ptr, PROTO_STS_G_X, sizeof(float), &imu_val.g[0]);
	sts_fill(pkt_buf, &ptr, PROTO_STS_G_Y, sizeof(float), &imu_val.g[1]);
	sts_fill(pkt_buf, &ptr, PROTO_STS_G_Z, sizeof(float), &imu_val.g[2]);

	static uint32_t incr_cnt = 0;
	sts_fill(pkt_buf, &ptr, PROTO_STS_INCR_CNT, sizeof(incr_cnt), &incr_cnt);
	incr_cnt++;

	uint8_t lp_on = safety_lsr_is_used_by_stm();
	sts_fill(pkt_buf, &ptr, PROTO_STS_LPC_ACT, sizeof(lp_on), &lp_on);

	uint8_t sd_detect = sd_detect_debounce.state & BTN_PRESS ? 1 : 0;
	sts_fill(pkt_buf, &ptr, PROTO_STS_SD_CARD_INSRT, sizeof(sd_detect), &sd_detect);

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

static bool parse_status(const uint8_t *pl, uint32_t size)
{
	for(uint32_t ptr = 0;;)
	{
		if(ptr > size) return 1; // error case
		if(ptr == size) return 0;

		switch(pl[ptr])
		{
		case PROTO_STS_SW_TYPE_BOOT:
			esp_sts.in_boot = true;
			ptr += 1 + 0;
			break;
		case PROTO_STS_SW_TYPE_FW:
			esp_sts.in_boot = false;
			ptr += 1 + 0;
			break;

		case PROTO_STS_LPC_ACT:
			esp_sts.lpc_on = pl[1 + ptr];
			ptr += 1 + sizeof(uint8_t);
			break;

		case PROTO_STS_GALV_LSR_PWR_EN:
			esp_sts.pwr_on = pl[1 + ptr];
			ptr += 1 + sizeof(uint8_t);
			break;

		case PROTO_STS_ERR:
			esp_sts.err = get_uint32(&pl[1 + ptr]);
			ptr += 1 + sizeof(uint32_t);
			break;

		case PROTO_STS_ERR_LATCHED:
			esp_sts.err_latched = get_uint32(&pl[1 + ptr]);
			ptr += 1 + sizeof(uint32_t);
			break;

		case PROTO_STS_INCR_CNT:
			esp_sts.cyc_cntr = get_uint32(&pl[1 + ptr]);
			ptr += 1 + sizeof(uint32_t);
			break;

		default: return 1; // error case
		}
	}
}

void proto_l1_parse(uint8_t cmd, const uint8_t *payload, uint16_t len)
{
	safety_reset_ser_to();

	switch(cmd)
	{
	case PROTO_CMD_STS:
		error_set(ERR_STM_PROTO, parse_status(payload, len));
		break;

	case PROTO_CMD_PARAM_SET:
		proto_update_params();
		break;

	default: send_b1(cmd, 1); break;
	}
}
