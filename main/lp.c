// #include "driver/spi_master.h"
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

spi_nodma_device_handle_t disp_spi = NULL;

#define PIN_LDAC 5
#define PIN_DATA 4
#define PIN_CLK 12
#define PIN_CS0 13
#define PIN_CS1 16

uint64_t timer_diff = 0;

volatile uint32_t gs_pnt_cnt[1] = {6};
volatile uint32_t gs_pnt_cur[1] = {0};

static intr_handle_t s_timer_handle;

#define DCMPS(x) ((x & 0xFF) << 8) | (x >> 8)
// volatile uint16_t daq_presets[] = {
// 	(1 << (15 - 8)) | (1 << (13 - 8)) | (1 << (12 - 8)) | DCMPS(4095 / 5),
// 	(0 << (15 - 8)) | (1 << (13 - 8)) | (1 << (12 - 8)) | DCMPS(4095 / 5 * 2),
// 	(0 << (15 - 8)) | (1 << (13 - 8)) | (1 << (12 - 8)) | DCMPS(4095 / 5 * 3),
// 	(1 << (15 - 8)) | (1 << (13 - 8)) | (1 << (12 - 8)) | DCMPS(4095 / 5 * 4),
// 	(0 << (15 - 8)) | (1 << (13 - 8)) | (1 << (12 - 8)) | DCMPS(4095 / 5 * 5),
// };
volatile uint16_t daq_presets[6 * 6] = {0};

volatile uint16_t presets[] = {0x1234, 0x5678,
							   0x9ABC, 0xDEF0,
							   0xA5A5};

#define DLY()                                  \
	for(volatile uint32_t x = 13; x != 0; x--) \
		asm("nop");

#define PIN_H(X) GPIO.out_w1ts = (1 << X)
#define PIN_L(X) GPIO.out_w1tc = (1 << X)

#define SPI_BUS HSPI_HOST
#define DAC_SPI SPI2
// #define SPI_BUS VSPI_HOST
// #define DAC_SPI SPI3

static IRAM_ATTR void
timer_isr(void *arg)
{
	TIMERG0.int_clr_timers.t0 = 1;
	TIMERG0.hw_timer[0].config.alarm_en = 1;

	// timer_set_counter_value(TIMER_GROUP_0, 1, 0);

	PIN_H(PIN_CS0);
	PIN_H(PIN_CS1);
	PIN_L(PIN_CS0);

	DAC_SPI.data_buf[0] = presets[0];
	DAC_SPI.cmd.usr = 1; // Start transfer
	// while(DAC_SPI.cmd.usr) // Wait for SPI bus ready
	// 	;
	DLY()

	PIN_H(PIN_CS0);
	PIN_H(PIN_CS1);
	PIN_L(PIN_CS0);

	DAC_SPI.data_buf[0] = presets[1];
	DAC_SPI.cmd.usr = 1; // Start transfer
	// while(DAC_SPI.cmd.usr) // Wait for SPI bus ready
	// 	;
	DLY()

	PIN_H(PIN_CS0);
	PIN_L(PIN_CS1);

	DAC_SPI.data_buf[0] = presets[2];
	DAC_SPI.cmd.usr = 1; // Start transfer
	// while(DAC_SPI.cmd.usr) // Wait for SPI bus ready
	// 	;
	DLY()

	PIN_H(PIN_CS1);
	PIN_H(PIN_CS0);
	PIN_L(PIN_CS1);

	DAC_SPI.data_buf[0] = presets[3];
	DAC_SPI.cmd.usr = 1; // Start transfer
	// while(DAC_SPI.cmd.usr) // Wait for SPI bus ready
	// 	;
	DLY()

	PIN_H(PIN_CS0);
	PIN_H(PIN_CS1);

	DAC_SPI.data_buf[0] = presets[4];
	DAC_SPI.cmd.usr = 1; // Start transfer
	// while(DAC_SPI.cmd.usr) // Wait for SPI bus ready
	// 	;
	DLY()

	PIN_L(PIN_CS0);
	PIN_L(PIN_CS1);

	PIN_L(PIN_LDAC);
	PIN_H(PIN_LDAC);

	// timer_diff = timer_group_get_counter_value_in_isr(TIMER_GROUP_0, 1);
}

void lp_init(void)
{
	gpio_reset_pin(PIN_LDAC);
	// gpio_reset_pin(PIN_DATA);
	// gpio_reset_pin(PIN_CLK);
	gpio_reset_pin(PIN_CS0);
	gpio_reset_pin(PIN_CS1);

	gpio_set_direction(PIN_LDAC, GPIO_MODE_OUTPUT);
	// gpio_set_direction(PIN_DATA, GPIO_MODE_OUTPUT);
	// gpio_set_direction(PIN_CLK, GPIO_MODE_OUTPUT);
	gpio_set_direction(PIN_CS0, GPIO_MODE_OUTPUT);
	gpio_set_direction(PIN_CS1, GPIO_MODE_OUTPUT);

	gpio_set_level(PIN_LDAC, 1);
	gpio_set_level(PIN_CS0, 1);
	gpio_set_level(PIN_CS1, 1);

	spi_nodma_device_handle_t spi;
	spi_nodma_bus_config_t buscfg = {
		.miso_io_num = -1,
		.mosi_io_num = PIN_DATA,
		.sclk_io_num = PIN_CLK,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1};

	spi_nodma_device_interface_config_t devcfg = {
		.clock_speed_hz = 16000000,
		.mode = 0,							// SPI mode 0
		.spics_io_num = -1,					// we will use external CS pin
		.spics_ext_io_num = -1,				// external CS pin
		.flags = 0 | SPI_DEVICE_HALFDUPLEX, // Set half duplex mode (Full duplex mode can also be set by commenting this line
											//  but we don't need full duplex in  this example
											//  also, SOME OF TFT FUNCTIONS ONLY WORKS IN HALF DUPLEX MODE
		.queue_size = 2,					// in some tft functions we are using DMA mode, so we need queues!
											// We can use pre_cb callback to activate DC, but we are handling DC in low-level functions, so it is not necessary
											//.pre_cb=ili_spi_pre_transfer_callback,  //Specify pre-transfer callback to handle D/C line
	};

	esp_err_t ret = spi_nodma_bus_add_device(SPI_BUS, &buscfg, &devcfg, &spi);
	assert(ret == ESP_OK);
	disp_spi = spi;

	// ret = spi_nodma_device_select(spi, 1);
	// assert(ret == ESP_OK);
	// ret = spi_nodma_device_deselect(spi);
	// assert(ret == ESP_OK);

	ret = spi_nodma_device_select(disp_spi, 1);
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
	// ESP_ERROR_CHECK(timer_isr_register(TIMER_GROUP_0, TIMER_0, &timer_isr, NULL, ESP_INTR_FLAG_IRAM, &s_timer_handle));
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

	{
		DAC_SPI.user.usr_mosi_highpart = 0;
		DAC_SPI.mosi_dlen.usr_mosi_dbitlen = 16 - 1;
		DAC_SPI.user.usr_mosi = 1;
		if(disp_spi->cfg.flags & SPI_DEVICE_HALFDUPLEX)
		{
			DAC_SPI.miso_dlen.usr_miso_dbitlen = 0;
			DAC_SPI.user.usr_miso = 0;
		}
		else
		{
			DAC_SPI.miso_dlen.usr_miso_dbitlen = 16 - 1;
			DAC_SPI.user.usr_miso = 1;
		}
	}

	memset((void *)daq_presets, 0, sizeof(daq_presets));
	for(uint32_t i = 0; i < 6; i++)
	{
		daq_presets[i * 6] = 30 + i * 5;
	}
	daq_presets[5 * 6] = 400;

	daq_presets[6 * 1 + 1] = 4095;
	daq_presets[6 * 2 + 2] = 4095;
	daq_presets[6 * 3 + 3] = 4095;
	daq_presets[6 * 4 + 4] = 4095;
	daq_presets[6 * 5 + 5] = 4095;

	// Disable unused channel
	PIN_H(PIN_CS0);
	PIN_H(PIN_CS1);
	DAC_SPI.data_buf[0] = (1 << (15 - 8)) | (0 << (12 - 8));
	DAC_SPI.cmd.usr = 1;
	while(DAC_SPI.cmd.usr) // Wait for SPI bus ready
		;
	PIN_L(PIN_CS0);

	timer_start(TIMER_GROUP_0, TIMER_0);
}
