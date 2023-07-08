// Copyright 2010-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef _DRIVER_SPI_MASTER_NODMA_H_
#define _DRIVER_SPI_MASTER_NODMA_H_

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "soc/spi_struct.h"

#include "esp32/rom/lldesc.h"
#include "esp_intr_alloc.h"

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * @brief Enum with the three SPI peripherals that are software-accessible in it
	 */
	typedef enum
	{
		SPI_HOST = 0,  ///< SPI1, SPI; Cannot be used in this driver!
		HSPI_HOST = 1, ///< SPI2, HSPI
		VSPI_HOST = 2  ///< SPI3, VSPI
	} spi_nodma_host_device_t;

	/**
	 * @brief This is a configuration structure for a SPI bus.
	 *
	 * You can use this structure to specify the GPIO pins of the bus. Normally, the driver will use the
	 * GPIO matrix to route the signals. An exception is made when all signals either can be routed through
	 * the IO_MUX or are -1. In that case, the IO_MUX is used, allowing for >40MHz speeds.
	 */
	typedef struct
	{
		int mosi_io_num;   ///< GPIO pin for Master Out Slave In (=spi_d) signal, or -1 if not used.
		int miso_io_num;   ///< GPIO pin for Master In Slave Out (=spi_q) signal, or -1 if not used.
		int sclk_io_num;   ///< GPIO pin for Spi CLocK signal, or -1 if not used.
		int quadwp_io_num; ///< GPIO pin for WP (Write Protect) signal which is used as D2 in 4-bit communication modes, or -1 if not used.
		int quadhd_io_num; ///< GPIO pin for HD (HolD) signal which is used as D3 in 4-bit communication modes, or -1 if not used.
	} spi_nodma_bus_config_t;

#define SPI_DEVICE_TXBIT_LSBFIRST (1 << 0)								   ///< Transmit command/address/data LSB first instead of the default MSB first
#define SPI_DEVICE_RXBIT_LSBFIRST (1 << 1)								   ///< Receive data LSB first instead of the default MSB first
#define SPI_DEVICE_BIT_LSBFIRST (SPI_TXBIT_LSBFIRST | SPI_RXBIT_LSBFIRST); ///< Transmit and receive LSB first
#define SPI_DEVICE_3WIRE (1 << 2)										   ///< Use spiq for both sending and receiving data
#define SPI_DEVICE_POSITIVE_CS (1 << 3)									   ///< Make CS positive during a transaction instead of negative
#define SPI_DEVICE_HALFDUPLEX (1 << 4)									   ///< Transmit data before receiving it, instead of simultaneously
#define SPI_DEVICE_CLK_AS_CS (1 << 5)									   ///< Output clock on CS line if CS is active

#define SPI_ERR_OTHER_CONFIG 7001

	typedef struct spi_nodma_transaction_t spi_nodma_transaction_t;
	typedef void (*transaction_cb_t)(spi_nodma_transaction_t *trans);

	/**
	 * @brief This is a configuration for a SPI slave device that is connected to one of the SPI buses.
	 */
	typedef struct
	{
		uint8_t command_bits;	  ///< Amount of bits in command phase (0-16)
		uint8_t address_bits;	  ///< Amount of bits in address phase (0-64)
		uint8_t dummy_bits;		  ///< Amount of dummy bits to insert between address and data phase
		uint8_t mode;			  ///< SPI mode (0-3)
		uint8_t duty_cycle_pos;	  ///< Duty cycle of positive clock, in 1/256th increments (128 = 50%/50% duty). Setting this to 0 (=not setting it) is equivalent to setting this to 128.
		uint8_t cs_ena_pretrans;  ///< Amount of SPI bit-cycles the cs should be activated before the transmission (0-16). This only works on half-duplex transactions.
		uint8_t cs_ena_posttrans; ///< Amount of SPI bit-cycles the cs should stay active after the transmission (0-16)
		int clock_speed_hz;		  ///< Clock speed, in Hz
		int spics_io_num;		  ///< CS GPIO pin for this device, handled by hardware; set to -1 if not used
		int spics_ext_io_num;	  ///< CS GPIO pin for this device, handled by software (spi_nodma_device_select/spi_nodma_device_deselect); only used if spics_io_num=-1
		uint32_t flags;			  ///< Bitwise OR of SPI_DEVICE_* flags
		int queue_size;			  ///< Transaction queue size. This sets how many transactions can be 'in the air' (queued using spi_device_queue_trans but not yet finished using spi_device_get_trans_result) at the same time
		transaction_cb_t pre_cb;  ///< Callback to be called before a transmission is started. This callback from 'spi_nodma_transfer_data' function.
		transaction_cb_t post_cb; ///< Callback to be called after a transmission has completed. This callback from 'spi_nodma_transfer_data' function.
		uint8_t selected;		  ///< **INTERNAL** 1 if the device's CS pin is active
	} spi_nodma_device_interface_config_t;

#define SPI_TRANS_MODE_DIO (1 << 0)			///< Transmit/receive data in 2-bit mode
#define SPI_TRANS_MODE_QIO (1 << 1)			///< Transmit/receive data in 4-bit mode
#define SPI_TRANS_MODE_DIOQIO_ADDR (1 << 2) ///< Also transmit address in mode selected by SPI_MODE_DIO/SPI_MODE_QIO
#define SPI_TRANS_USE_RXDATA (1 << 3)		///< Receive into rx_data member of spi_nodma_transaction_t instead into memory at rx_buffer.
#define SPI_TRANS_USE_TXDATA (1 << 4)		///< Transmit tx_data member of spi_nodma_transaction_t instead of data at tx_buffer. Do not set tx_buffer when using this.

	/**
	 * This structure describes one SPI transmission
	 */
	struct spi_nodma_transaction_t
	{
		uint32_t flags;	  ///< Bitwise OR of SPI_TRANS_* flags
		uint16_t command; ///< Command data. Specific length was given when device was added to the bus.
		uint64_t address; ///< Address. Specific length was given when device was added to the bus.
		size_t length;	  ///< Total data length to be transmitted to the device, in bits; if 0, no data is transmitted
		size_t rxlength;  ///< Total data length to be received from the device, in bits; if 0, no data is received
		void *user;		  ///< User-defined variable. Can be used to store eg transaction ID or data to be used by pre_cb and/or post_cb callbacks.
		union
		{
			const void *tx_buffer; ///< Pointer to transmit buffer, or NULL for no MOSI phase
			uint8_t tx_data[4];	   ///< If SPI_USE_TXDATA is set, data set here is sent directly from this variable.
		};
		union
		{
			void *rx_buffer;	///< Pointer to receive buffer, or NULL for no MISO phase
			uint8_t rx_data[4]; ///< If SPI_USE_RXDATA is set, data is received directly to this variable
		};
	};

#define NO_CS 3					// Number of CS pins per SPI host
#define NO_DEV 6				// Number of spi devices per SPI host; more than 3 devices can be attached to the same bus if using software CS's
#define SPI_SEMAPHORE_WAIT 2000 // Time in ms to wait for SPI mutex

	typedef struct spi_nodma_device_t spi_nodma_device_t;

	typedef struct
	{
		spi_nodma_device_t *device[NO_DEV];
		intr_handle_t intr;
		spi_dev_t *hw;
		spi_nodma_transaction_t *cur_trans;
		int cur_device;
		lldesc_t dmadesc_tx, dmadesc_rx;
		bool no_gpio_matrix;
		QueueHandle_t spi_nodma_bus_mutex;
		spi_nodma_bus_config_t cur_bus_config;
	} spi_nodma_host_t;

	struct spi_nodma_device_t
	{
		QueueHandle_t trans_queue;
		QueueHandle_t ret_queue;
		spi_nodma_device_interface_config_t cfg;
		spi_nodma_host_t *host;
		spi_nodma_bus_config_t bus_config;
		spi_nodma_host_device_t host_dev;
	};

	typedef struct spi_nodma_device_t *spi_nodma_device_handle_t; ///< Handle for a device on a SPI bus
	typedef struct spi_nodma_host_t *spi_nodma_host_handle_t;
	typedef struct spi_nodma_device_interface_config_t *spi_nodma_device_interface_config_handle_t;

	esp_err_t spi_nodma_bus_add_device(spi_nodma_host_device_t host, spi_nodma_bus_config_t *bus_config, spi_nodma_device_interface_config_t *dev_config, spi_nodma_device_handle_t *handle);
	esp_err_t spi_nodma_device_select(spi_nodma_device_handle_t handle, int force);
	esp_err_t spi_nodma_device_deselect(spi_nodma_device_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif