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

volatile uint16_t daq_presets[6 * 6] = {0};

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

	timer_config_t config_rt = {
		.alarm_en = true,
		.counter_en = false,
		.intr_type = TIMER_INTR_LEVEL,
		.counter_dir = TIMER_COUNT_UP,
		.auto_reload = true,
		.divider = 2, // Set prescaler for 40 MHz clock
	};

	timer_init(TIMER_GROUP_0, 1, &config_rt);
	timer_set_counter_value(TIMER_GROUP_0, 1, 0);
	timer_set_alarm_value(TIMER_GROUP_0, 1, 5000000);
	timer_enable_intr(TIMER_GROUP_0, 1);
	timer_start(TIMER_GROUP_0, 1);

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
	}

#define p_l 2048 - 0 * 100;
#define p_h 2048 + 0 * 100;

	daq_presets[6 * 0 + 1] = p_l; // X
	daq_presets[6 * 0 + 2] = p_l; // Y

	daq_presets[6 * 1 + 1] = p_l; // X
	daq_presets[6 * 1 + 2] = p_h; // Y

	daq_presets[6 * 2 + 1] = p_h; // X
	daq_presets[6 * 2 + 2] = p_h; // Y

	daq_presets[6 * 3 + 1] = p_h; // X
	daq_presets[6 * 3 + 2] = p_l; // Y

	daq_presets[6 * 1 + 3] = 0 + 0 * 4095; // R
	daq_presets[6 * 1 + 4] = 0 + 0 * 4095; // G
	daq_presets[6 * 1 + 5] = 0 + 0 * 4095; // B

	// daq_presets[5 * 6] = 400;

	// daq_presets[6 * 1 + 1] = 4095;
	// daq_presets[6 * 2 + 2] = 4095;
	// daq_presets[6 * 3 + 3] = 4095;
	// daq_presets[6 * 4 + 4] = 4095;
	// daq_presets[6 * 5 + 5] = 4095;

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
}

// QLPE - quick laser projector engine

bool lsr_q_is_circular = false;
void *frame_first = NULL;
void *frame_current = NULL; // used by QLPE
void *frame_next = NULL;	// used by QLPE
void *frame_pending = NULL;
uint32_t frame_count = 0; // including preparation frames

enum
{
	LP_NO_MEM = 1,
	LP_FRAME_NOT_CREATED,
};

void lsr_q_clear(void)
{
	frame_current = NULL; // this will stop QLPE

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
		if(frame_pending) free(frame_pending); // also delete preparation frames
	}

	frame_first = NULL;
	frame_next = NULL;
	frame_pending = NULL;
	lsr_q_is_circular = false;
	frame_count = 0;
}

int lsr_q_reserve_frame(uint32_t point_count)
{
	// frame_pending = malloc(sizeof(lframe_header_t) + point_count * sizeof(ilda_point_t));
	// if(!frame_pending) return LP_NO_MEM;

	// frame_count++;
	return 0;
}

int lsr_q_modify_point(uint32_t point)
{
	if(frame_pending == NULL) return LP_FRAME_NOT_CREATED;

	return 0;
}

int lsr_q_add_frame(void) // finalizes the preparation frame
{
	if(frame_pending == NULL) return LP_FRAME_NOT_CREATED;

	frame_next = frame_pending;
	frame_pending = NULL;

	if(!frame_current) frame_current = frame_pending; // this will start the scan

	return 0;
}

void lsr_q_set_circular(bool state) { lsr_q_is_circular = state; }