#include "serial.h"
#include "cobs.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "proto.h"
#include "proto_l0.h"
#include "sdkconfig.h"

#define UART UART_NUM_1
#define BUF_SIZE (512)

static uint8_t buf_cobs_encode[PROTO_MAX_PKT_SIZE + 2];
static uint8_t rx_buffer[BUF_SIZE];

static QueueHandle_t uart0_queue;

static void ser_rx_task(void *arg)
{
	uart_event_t event;
	for(;;)
	{
		if(xQueueReceive(uart0_queue, (void *)&event, (TickType_t)portMAX_DELAY))
		{
			switch(event.type)
			{
			case UART_DATA:
				int rdd = uart_read_bytes(UART, rx_buffer, event.size, portMAX_DELAY);
				// ESP_LOGI("", "[DATA EVT]: %d %d", rdd, event.size);
				proto_append(rx_buffer, rdd);
				proto_poll();
				break;

			case UART_FIFO_OVF:
				ESP_LOGI("", "hw fifo overflow");
				uart_flush_input(UART);
				xQueueReset(uart0_queue);
				break;

			case UART_BUFFER_FULL:
				ESP_LOGI("", "ring buffer full");
				uart_flush_input(UART);
				xQueueReset(uart0_queue);
				break;

				// case UART_BREAK:
				// 	ESP_LOGI("", "uart rx break");
				// 	break;

				// case UART_PARITY_ERR:
				// 	ESP_LOGI("", "uart parity error");
				// 	break;

				// case UART_FRAME_ERR:
				// 	ESP_LOGI("", "uart frame error");
				// 	break;

			case UART_PATTERN_DET:
				size_t buffered_size;
				uart_get_buffered_data_len(UART, &buffered_size);
				int pos = uart_pattern_pop_pos(UART);
				// ESP_LOGI("", "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
				if(pos == -1)
				{
					uart_flush_input(UART);
				}
				else
				{
					int rdd = uart_read_bytes(UART, rx_buffer, pos, 100 / portTICK_PERIOD_MS);
					// ESP_LOGI("", "[PAT EVT]: %d", rdd);
					proto_append(rx_buffer, rdd);
					proto_poll();
				}
				break;

			default:
				ESP_LOGI("", "uart event type: %d", event.type);
				break;
			}
		}
	}
}

void serial_init(void)
{
	uart_config_t uart_config = {
		.baud_rate = 2000000,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.rx_flow_ctrl_thresh = 120,
		.source_clk = UART_SCLK_DEFAULT,
	};

	ESP_ERROR_CHECK(uart_driver_install(UART, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart0_queue, 0));
	ESP_ERROR_CHECK(uart_param_config(UART, &uart_config));
	ESP_ERROR_CHECK(uart_set_pin(UART, 33, 32, -1, -1));

	uart_enable_pattern_det_baud_intr(UART, 0, 1, 200, 0, 0);
	uart_pattern_queue_reset(UART, 256); // Reset the pattern queue length to record at most 20 pattern positions

	xTaskCreate(ser_rx_task, "ser_rx", 4096, NULL, 12, NULL);
}

void serial_tx(const uint8_t *data, uint32_t len)
{
	if(len + 2 > PROTO_MAX_PKT_SIZE + 2) return;
	uint32_t sz = 1 + cobs_encode(data, len, &buf_cobs_encode[1]);
	uart_wait_tx_done(UART, 1000);
	uart_tx_chars(UART, (const char *)buf_cobs_encode, sz);
}