#include "eth_smi_iic.h"
#include "accel.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_rom_gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "hal/emac_hal.h"
#include "hal/gpio_hal.h"
#include <math.h>
#include <stdatomic.h>
#include <sys/cdefs.h>

#define ACCEL_MARGIN 2000

SemaphoreHandle_t smi_iic_mutex;
atomic_bool pos_fix_failure = false;
static int16_t imu_acc_fix[3];

i2c_config_t iic_conf = {
	.mode = I2C_MODE_MASTER,
	.sda_io_num = 18,
	.scl_io_num = 23,
	.sda_pullup_en = GPIO_PULLUP_ENABLE,
	.scl_pullup_en = GPIO_PULLUP_ENABLE,
	.master.clk_speed = 400000,
};

typedef enum
{
	ESP_ETH_FSM_STOP,
	ESP_ETH_FSM_START
} esp_eth_fsm_t;

typedef struct
{
	esp_eth_mediator_t mediator;
	esp_eth_phy_t *phy;
	esp_eth_mac_t *mac;
	esp_timer_handle_t check_link_timer;
	uint32_t check_link_period_ms;
	eth_speed_t speed;
	eth_duplex_t duplex;
	eth_link_t link;
	atomic_int ref_count;
	void *priv;
	_Atomic esp_eth_fsm_t fsm;
	esp_err_t (*stack_input)(esp_eth_handle_t eth_handle, uint8_t *buffer, uint32_t length, void *priv);
	esp_err_t (*on_lowlevel_init_done)(esp_eth_handle_t eth_handle);
	esp_err_t (*on_lowlevel_deinit_done)(esp_eth_handle_t eth_handle);
	esp_err_t (*customized_read_phy_reg)(esp_eth_handle_t eth_handle, uint32_t phy_addr, uint32_t phy_reg, uint32_t *reg_value);
	esp_err_t (*customized_write_phy_reg)(esp_eth_handle_t eth_handle, uint32_t phy_addr, uint32_t phy_reg, uint32_t reg_value);
} esp_eth_drivers_t;

typedef struct
{
	esp_eth_mac_t parent;
	esp_eth_mediator_t *eth;
	emac_hal_context_t hal;
	intr_handle_t intr_hdl;
	TaskHandle_t rx_task_hdl;
	uint32_t sw_reset_timeout_ms;
	uint32_t frames_remain;
	uint32_t free_rx_descriptor;
	uint32_t flow_control_high_water_mark;
	uint32_t flow_control_low_water_mark;
	int smi_mdc_gpio_num;
	int smi_mdio_gpio_num;
} emac_esp32__t;

static void gpio_remap_smi(emac_esp32__t *emac)
{
	// SMI MDC
	PIN_INPUT_DISABLE(GPIO_PIN_MUX_REG[emac->smi_mdc_gpio_num]);
	gpio_ll_output_enable(&GPIO, emac->smi_mdc_gpio_num);
	gpio_ll_od_disable(&GPIO, emac->smi_mdc_gpio_num);

	esp_rom_gpio_connect_out_signal(emac->smi_mdc_gpio_num, EMAC_MDC_O_IDX, false, false);
	REG_SET_FIELD(GPIO_PIN_MUX_REG[emac->smi_mdc_gpio_num], MCU_SEL, PIN_FUNC_GPIO);

	// SMI MDIO
	PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[emac->smi_mdio_gpio_num]);
	gpio_ll_output_enable(&GPIO, emac->smi_mdio_gpio_num);
	gpio_ll_od_disable(&GPIO, emac->smi_mdio_gpio_num);

	esp_rom_gpio_connect_out_signal(emac->smi_mdio_gpio_num, EMAC_MDO_O_IDX, false, false);
	esp_rom_gpio_connect_in_signal(emac->smi_mdio_gpio_num, EMAC_MDI_I_IDX, false);
	REG_SET_FIELD(GPIO_PIN_MUX_REG[emac->smi_mdio_gpio_num], MCU_SEL, PIN_FUNC_GPIO);
}

static void gpio_remap_iic(void)
{
	// SCL
	gpio_ll_set_level(&GPIO, iic_conf.scl_io_num, 1);
	REG_SET_FIELD(GPIO_PIN_MUX_REG[iic_conf.scl_io_num], MCU_SEL, PIN_FUNC_GPIO);
	PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[iic_conf.scl_io_num]);
	gpio_ll_output_enable(&GPIO, iic_conf.scl_io_num);
	gpio_ll_od_disable(&GPIO, iic_conf.scl_io_num);
	esp_rom_gpio_connect_out_signal(iic_conf.scl_io_num, I2CEXT0_SCL_OUT_IDX, 0, 0);
	esp_rom_gpio_connect_in_signal(iic_conf.scl_io_num, I2CEXT0_SCL_IN_IDX, 0);
	// gpio_pulldown_dis(iic_conf.scl_io_num);
	// gpio_pullup_en(iic_conf.scl_io_num);

	// SDA
	gpio_ll_set_level(&GPIO, iic_conf.sda_io_num, 1);
	REG_SET_FIELD(GPIO_PIN_MUX_REG[iic_conf.sda_io_num], MCU_SEL, PIN_FUNC_GPIO);
	PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[iic_conf.sda_io_num]);
	gpio_ll_output_enable(&GPIO, iic_conf.sda_io_num);
	gpio_ll_od_enable(&GPIO, iic_conf.sda_io_num);
	gpio_pulldown_dis(iic_conf.sda_io_num);
	gpio_pullup_en(iic_conf.sda_io_num);
	esp_rom_gpio_connect_out_signal(iic_conf.sda_io_num, I2CEXT0_SDA_OUT_IDX, 0, 0);
	esp_rom_gpio_connect_in_signal(iic_conf.sda_io_num, I2CEXT0_SDA_IN_IDX, 0);
}

esp_err_t custom_read_phy_reg(esp_eth_handle_t h, uint32_t phy_addr, uint32_t phy_reg, uint32_t *reg_value)
{
	esp_eth_drivers_t *eth_driver = (esp_eth_drivers_t *)h;
	esp_eth_mac_t *mac = eth_driver->mac;
	emac_esp32__t *emac = __containerof(mac, emac_esp32__t, parent);

	if(xSemaphoreTake(smi_iic_mutex, portMAX_DELAY))
	{
		gpio_remap_smi(emac);
		esp_err_t err = mac->read_phy_reg(mac, phy_addr, phy_reg, reg_value);
		xSemaphoreGive(smi_iic_mutex);
		return err;
	}
	return ESP_ERR_TIMEOUT;
}

esp_err_t custom_write_phy_reg(esp_eth_handle_t h, uint32_t phy_addr, uint32_t phy_reg, uint32_t reg_value)
{
	esp_eth_drivers_t *eth_driver = (esp_eth_drivers_t *)h;
	esp_eth_mac_t *mac = eth_driver->mac;
	emac_esp32__t *emac = __containerof(mac, emac_esp32__t, parent);

	if(xSemaphoreTake(smi_iic_mutex, portMAX_DELAY))
	{
		gpio_remap_smi(emac);
		esp_err_t err = mac->write_phy_reg(mac, phy_addr, phy_reg, reg_value);
		xSemaphoreGive(smi_iic_mutex);
		return err;
	}
	return ESP_ERR_TIMEOUT;
}

void imu_reset_fixture(void)
{
	if(xSemaphoreTake(smi_iic_mutex, portMAX_DELAY))
	{
		gpio_remap_iic();
		imu_get_acc(imu_acc_fix);
		pos_fix_failure = false;
		ESP_LOGI("", "New 3D pos fixture: %d %d %d", imu_acc_fix[0], imu_acc_fix[1], imu_acc_fix[2]);
		xSemaphoreGive(smi_iic_mutex);
	}
}

void imu_task(void *pvParameter)
{
	if(!xSemaphoreTake(smi_iic_mutex, portMAX_DELAY)) return;
	{
		gpio_remap_iic();
		i2c_param_config(0, &iic_conf);
		i2c_driver_install(0, I2C_MODE_MASTER, 0, 0, 0);
		imu_init();
		xSemaphoreGive(smi_iic_mutex);
	}

	vTaskDelay(5);

	imu_reset_fixture();

	while(1)
	{
		int16_t imu_acc[3];
		if(xSemaphoreTake(smi_iic_mutex, portMAX_DELAY))
		{
			gpio_remap_iic();
			imu_get_acc(imu_acc);
			xSemaphoreGive(smi_iic_mutex);

			if(!pos_fix_failure && (imu_acc[0] || imu_acc[1] || imu_acc[2]))
			{
				if(abs(imu_acc[0] - imu_acc_fix[0]) > ACCEL_MARGIN ||
				   abs(imu_acc[1] - imu_acc_fix[1]) > ACCEL_MARGIN ||
				   abs(imu_acc[2] - imu_acc_fix[2]) > ACCEL_MARGIN)
				{
					pos_fix_failure = true;
					ESP_LOGE("", "3D pos fixture failure! %d %d %d", imu_acc[0], imu_acc[1], imu_acc[2]);
				}
			}
		}

		vTaskDelay(2);
	}
}

void eth_smii_config(esp_eth_handle_t h)
{
	smi_iic_mutex = xSemaphoreCreateMutex();

	esp_eth_drivers_t *eth_driver = (esp_eth_drivers_t *)h;
	eth_driver->customized_read_phy_reg = custom_read_phy_reg;
	eth_driver->customized_write_phy_reg = custom_write_phy_reg;

#ifndef CONFIG_FREERTOS_UNICORE
	xTaskCreatePinnedToCore(imu_task, "imu_task", 8000, NULL, 1, NULL, 1);
#else
	xTaskCreate(imu_task, "imu_task", 8000, NULL, 1, NULL);
#endif
}