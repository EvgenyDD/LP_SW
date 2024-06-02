#include "proto_l0.h"
#include "cobs.h"
#include "crc16_ccitt.h"
#include "proto.h"
#include "serial.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define RX_BUF_SZ (6 + PROTO_MAX_PKT_SIZE)
static volatile uint8_t rx_buf[RX_BUF_SZ];
static volatile uint16_t rx_ptr_tail = 0, rx_ptr_head = 0;

static uint8_t buf_cobs_acc[300];
static uint8_t buf_cobs_decode[300];
static uint16_t buf_cobs_acc_sz = 0;

void proto_calc_fill_crc16(uint8_t *data, uint32_t len)
{
	uint16_t crc_value = crc16_ccitt(data, len, 0xFFFF);
	memcpy(&data[len], &crc_value, sizeof(crc_value));
}

void proto_append(const volatile uint8_t *data, uint32_t len)
{
	for(uint32_t i = 0; i < len; i++)
	{
		rx_buf[rx_ptr_head] = data[i];
		rx_ptr_head = (rx_ptr_head + 1) % RX_BUF_SZ;
	}
}

static void try_parse(const uint8_t *buf, uint16_t sz_buf)
{
	if(sz_buf >= 4)
	{
		uint16_t sz_pkt = ((uint16_t)buf[1] << 8L) | buf[0];
		if(sz_pkt < 6L || sz_pkt > PROTO_MAX_PKT_SIZE)
		{
			printf("wtf2\n");
			return;
		}

		if(sz_buf == sz_pkt)
		{
			uint16_t crcv = ((uint16_t)buf[sz_pkt - 1L] << 8L) | buf[sz_pkt - 2L];
			if(crc16_ccitt(buf, sz_pkt - 2, 0xFFFFL) == crcv) // crc match
			{
				proto_l1_parse(buf[2],	/* cmd */
							   &buf[3], /* pl */
							   sz_pkt - 5L);
			}
			else
			{
				printf("wtf3\n");
			}
		}
	}
}

void proto_poll(void)
{
	while(rx_ptr_tail != rx_ptr_head)
	{
		if(rx_buf[rx_ptr_tail] == 0) // as end/start marker
		{
			if(buf_cobs_acc_sz != 0)
			{
				unsigned int sz = cobs_decode(buf_cobs_acc, buf_cobs_acc_sz, buf_cobs_decode);
				try_parse(buf_cobs_decode, sz);
				buf_cobs_acc_sz = 0;
			}
		}
		else
		{
			buf_cobs_acc[buf_cobs_acc_sz++] = rx_buf[rx_ptr_tail];
			if(buf_cobs_acc_sz >= sizeof(buf_cobs_acc))
			{
				buf_cobs_acc_sz = 0;
				printf("wtf1\n");
			}
		}
		rx_ptr_tail = (rx_ptr_tail + 1) % RX_BUF_SZ;
	}
}