#include "lp.h"
#include "driver/gpio.h"
#include "driver/timer.h"
#include "esp_attr.h"
#include "esp_eth_mac.h"
#include "esp_eth_phy.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ilda.h"
#include "spi_master_nodma.h"
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

#define PIN_LDAC 5
#define PIN_DATA 4
#define PIN_CLK 12
#define PIN_COUNT 13

#define SPI_BUS HSPI_HOST
#define DAC_SPI SPI2

#define PIN_H(X) GPIO.out_w1ts = (1 << X)
#define PIN_L(X) GPIO.out_w1tc = (1 << X)

spi_nodma_device_handle_t lsr_spi = NULL;

volatile uint32_t gs_pnt_cnt[1] = {1};
volatile uint32_t gs_pnt_cur[1] = {0};
volatile uint32_t gs_sw[1] = {0};

volatile uint32_t lp_image_cnt_points = 1;

volatile uint16_t daq_presets[6 * 4000] = {0};
volatile uint16_t daq_presets2[6 * 10] = {0};

volatile uint8_t default_frame[sizeof(lp_frame_h_t) + 1 * sizeof(lp_point_t)];

void lp_init(void)
{
	gpio_reset_pin(PIN_LDAC);
	// gpio_reset_pin(PIN_DATA);
	// gpio_reset_pin(PIN_CLK);
	gpio_reset_pin(PIN_COUNT);

	gpio_set_direction(PIN_LDAC, GPIO_MODE_OUTPUT);
	// gpio_set_direction(PIN_DATA, GPIO_MODE_OUTPUT);
	// gpio_set_direction(PIN_CLK, GPIO_MODE_OUTPUT);
	gpio_set_direction(PIN_COUNT, GPIO_MODE_OUTPUT);
	gpio_set_level(PIN_LDAC, 1);
	gpio_set_level(PIN_COUNT, 0);

	spi_nodma_device_handle_t spi;
	spi_nodma_bus_config_t buscfg = {
		.miso_io_num = -1,
		.mosi_io_num = PIN_DATA,
		.sclk_io_num = PIN_CLK,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1};

	spi_nodma_device_interface_config_t devcfg = {
		.clock_speed_hz = 16000000,
		.mode = 0,
		.spics_io_num = -1,
		.spics_ext_io_num = -1,
		.flags = 0 | SPI_DEVICE_HALFDUPLEX,
		.queue_size = 2,
	};

	esp_err_t ret = spi_nodma_bus_add_device(SPI_BUS, &buscfg, &devcfg, &spi);
	assert(ret == ESP_OK);
	lsr_spi = spi;

	ret = spi_nodma_device_select(lsr_spi, 1);
	assert(ret == ESP_OK);

	timer_config_t config = {
		.alarm_en = true,
		.counter_en = false,
		.intr_type = TIMER_INTR_LEVEL,
		.counter_dir = TIMER_COUNT_UP,
		.auto_reload = true,
		.divider = 80 /* 1 us per tick */
	};
	timer_init(TIMER_GROUP_0, TIMER_0, &config);
	timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
	timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, 22);
	timer_enable_intr(TIMER_GROUP_0, TIMER_0);
	ESP_ERROR_CHECK(timer_isr_register(TIMER_GROUP_0, TIMER_0, NULL, NULL, ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LEVEL5, NULL));

	// timer_config_t config_rt = {
	// 	.alarm_en = true,
	// 	.counter_en = false,
	// 	.intr_type = TIMER_INTR_LEVEL,
	// 	.counter_dir = TIMER_COUNT_UP,
	// 	.auto_reload = true,
	// 	.divider = 2, // Set prescaler for 40 MHz clock
	// };

	// timer_init(TIMER_GROUP_0, 1, &config_rt);
	// timer_set_counter_value(TIMER_GROUP_0, 1, 0);
	// timer_set_alarm_value(TIMER_GROUP_0, 1, 5000000);
	// timer_enable_intr(TIMER_GROUP_0, 1);
	// timer_start(TIMER_GROUP_0, 1);

	DAC_SPI.user.usr_mosi_highpart = 0;
	DAC_SPI.mosi_dlen.usr_mosi_dbitlen = 16 - 1;
	DAC_SPI.user.usr_mosi = 1;
	if(lsr_spi->cfg.flags & SPI_DEVICE_HALFDUPLEX)
	{
		DAC_SPI.miso_dlen.usr_miso_dbitlen = 0;
		DAC_SPI.user.usr_miso = 0;
	}
	else
	{
		DAC_SPI.miso_dlen.usr_miso_dbitlen = 16 - 1;
		DAC_SPI.user.usr_miso = 1;
	}

	memset((void *)daq_presets, 0, sizeof(daq_presets));

	for(uint32_t i = 0; i < 6; i++)
	{
		daq_presets[i * 6] = 1000;
		daq_presets2[i * 6] = 1000;
	}

#define p_l 2048 - 300
#define p_h 2048 + 300

	daq_presets[6 * 0 + 1] = 2048;			// X
	daq_presets[6 * 0 + 2] = 2048;			// Y
	daq_presets[6 * 1 + 3] = 0 + 0 * 4095;	// G
	daq_presets[6 * 1 + 4] = 0 + 0 * 4095;	// B
	daq_presets[6 * 1 + 5] = 0 + 0 * 4095;	// R

	daq_presets2[6 * 0 + 1] = 2048;			// X
	daq_presets2[6 * 0 + 2] = 2048;			// Y
	daq_presets2[6 * 1 + 3] = 0 + 0 * 4095; // G
	daq_presets2[6 * 1 + 4] = 0 + 0 * 4095; // B
	daq_presets2[6 * 1 + 5] = 0 + 0 * 4095; // R

	lp_off();

	memset((void *)default_frame, 0, sizeof(default_frame));
	lp_frame_h_t *i_hdr = (lp_frame_h_t *)default_frame;
	lp_point_t *i_pnt = (lp_point_t *)&default_frame[sizeof(ilda_header_t)];
	i_hdr->p_frame_next = NULL;
	i_hdr->repeat_cnt = 0;
	i_hdr->point_cnt = 1;
	i_hdr->point_cur = 0;
	i_pnt->time_us = 1000;
	i_pnt->x = 2048;
	i_pnt->y = 2048;

	// disable unused DAC channel
	PIN_L(PIN_LDAC);
	PIN_H(PIN_LDAC);
	for(uint32_t i = 0; i < 4; i++)
	{
		PIN_H(PIN_COUNT);
		PIN_L(PIN_COUNT);
	}
	DAC_SPI.data_buf[0] = (1 << (15 - 8)) | (0 << (12 - 8));
	DAC_SPI.cmd.usr = 1;
	while(DAC_SPI.cmd.usr) // Wait for SPI bus ready
		;
	PIN_H(PIN_COUNT);

	timer_start(TIMER_GROUP_0, TIMER_0);

	lp_fill_image();
}

// ===============================================================
// ===============================================================
// ===============================================================
// ===============================================================
// ===============================================================

// LPRT - laser projector realtime loop

void *frame_first = NULL; // to clear queue
void volatile *frame_current = NULL;
void *frame_edit = NULL;

enum
{
	LP_NO_MEM = 1,
	LP_FRAME_NOT_CREATED,
};

void lp_fill_image(void)
{
	lp_square(CLR_WHT);
}

void lp_off(void)
{
	gs_pnt_cnt[0] = 1;
	gs_sw[0] = 0;
}

void lp_on(void)
{
	gs_pnt_cnt[0] = lp_image_cnt_points;
	gs_sw[0] = 1;
}

float map(float x, float in_min, float in_max, float out_min, float out_max)
{
	if(x < in_min)
	{
		x = in_min;
	}
	if(x > in_max)
	{
		x = in_max;
	}
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
void lp_fb_reinit(void)
{
	lp_image_cnt_points = 0;
}

// us=83 for 12K
void lp_fb_append(float x, float y, float cr, float cg, float cb, float scale, float max_br, uint32_t us)
{
	daq_presets[6 * lp_image_cnt_points + 0] = us;
	daq_presets[6 * lp_image_cnt_points + COORD_X] = map(x * scale, -32767, 32767, 0, 4095);
	daq_presets[6 * lp_image_cnt_points + COORD_Y] = map(y * scale, -32767, 32767, 4095, 0);
	daq_presets[6 * lp_image_cnt_points + CLR_R] = map(cr * max_br, 0, 255, 0, 4095);
	daq_presets[6 * lp_image_cnt_points + CLR_G] = map(cg * max_br, 0, 255, 0, 4095);
	daq_presets[6 * lp_image_cnt_points + CLR_B] = map(cb * max_br, 0, 255, 0, 4095);
	lp_image_cnt_points++;
}

void lp_square(uint8_t color)
{
	// daq_presets[6 * 0 + 1] = 0; // X
	// daq_presets[6 * 0 + 2] = 0; // Y

	// daq_presets[6 * 1 + 1] = 0;	   // X
	// daq_presets[6 * 1 + 2] = 4095; // Y

	// daq_presets[6 * 2 + 1] = 4095; // X
	// daq_presets[6 * 2 + 2] = 4095; // Y

	// daq_presets[6 * 3 + 1] = 4095; // X
	// daq_presets[6 * 3 + 2] = 0;	   // Y

#define VVV 200

#define pos_l_x 2048 - VVV * 2 / 3
#define pos_h_x 2048 + VVV * 2 / 3

#define pos_l_y 2048 - VVV
#define pos_h_y 2048 + VVV

	daq_presets[6 * 0 + 1] = pos_l_x; // X
	daq_presets[6 * 0 + 2] = pos_l_y; // Y

	daq_presets[6 * 1 + 1] = pos_l_x; // X
	daq_presets[6 * 1 + 2] = pos_h_y; // Y

	daq_presets[6 * 2 + 1] = pos_h_x; // X
	daq_presets[6 * 2 + 2] = pos_h_y; // Y

	daq_presets[6 * 3 + 1] = pos_h_x; // X
	daq_presets[6 * 3 + 2] = pos_l_y; // Y

#define LVL 600
	for(uint32_t frm = 0; frm < 4; frm++)
	{
		daq_presets[6 * frm + CLR_R] = (color == CLR_R || color == CLR_WHT) ? 600 : 0; // R
		daq_presets[6 * frm + CLR_G] = (color == CLR_G || color == CLR_WHT) ? LVL : 0; // G
		daq_presets[6 * frm + CLR_B] = (color == CLR_B || color == CLR_WHT) ? LVL : 0; // B
		daq_presets[6 * frm] = 5000;
	}
	lp_image_cnt_points = 4;
}

// void lp_blank(void)
// {
// 	memset((void *)daq_presets, 0, sizeof(daq_presets));
// 	daq_presets[6 * 0 + 1] = 2048; // X
// 	daq_presets[6 * 0 + 2] = 2048; // Y
// 	for(uint32_t i = 0; i < 6; i++)
// 	{
// 		daq_presets[i * 6] = 8000;
// 	}
// 	gs_pnt_cnt[0] = 1;
// }

void lsr_q_clear(void)
{
	frame_current = default_frame;

	// clear the queue
	if(!frame_first)
	{
		while(1)
		{
			void *next;
			memcpy(&next, &frame_first, 4);
			free(frame_first);
			if(!next) break;
		}
		if(frame_edit) free(frame_edit); // also delete preparation frames
	}

	frame_first = NULL;
	frame_edit = NULL;
}

int lp_frame_add(uint32_t point_count) // allocate frame buffer
{
	// frame_edit = malloc(sizeof(lframe_header_t) + point_count * sizeof(lp_point_t));
	// if(!frame_edit) return LP_NO_MEM;

	return 0;
}

int lsr_q_modify_point(uint32_t point)
{
	if(frame_edit == NULL) return LP_FRAME_NOT_CREATED;

	return 0;
}

int lp_frame_append(void) // append frame to the projection queue
{
	if(frame_edit == NULL) return LP_FRAME_NOT_CREATED;

	// frame_next = frame_edit;
	frame_edit = NULL;

	if(!frame_current) frame_current = frame_edit; // this will start the scan

	return 0;
}