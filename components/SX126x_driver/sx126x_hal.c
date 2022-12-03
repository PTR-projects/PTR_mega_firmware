/*
 * SX1262_driver.c
 *
 *  Created on: 22 maj 2019
 *      Author: b.moczala
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include <driver/spi_master.h>
#include <string.h>
#include "BOARD.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "SX126x_hal.h"

static const char *TAG = "sx126x driver";
static spi_device_handle_t spi_dev_handle_SX126X;

void SX126X_initIO(){
	gpio_reset_pin(RF_BUSY_PIN);
	gpio_reset_pin(RF_RST_PIN);

	gpio_set_direction(RF_BUSY_PIN, GPIO_MODE_INPUT);
	gpio_set_direction(RF_RST_PIN,  GPIO_MODE_OUTPUT);

	sx126x_hal_reset(0);
}

esp_err_t SX126X_spi_init(void)
{
	esp_err_t ret = ESP_OK;

/*  SPI BUS INITIALIZATION */
// Uncomment if not initialised elsewhere
/*
	spi_bus_config_t buscfg={
		.miso_io_num   = SPI_MISO_PIN,
		.mosi_io_num   = SPI_MOSI_PIN,
		.sclk_io_num   = SPI_SCK_PIN,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,

	};

	ret = spi_bus_initialize(SPI_BUS, &buscfg, SPI_DMA_CH_AUTO); //Initialize the SPI bus (prev: SPI_DMA_CH_AUTO)
	ESP_ERROR_CHECK(ret);
*/


/* CONFIGURE SPI DEVICE */

spi_device_interface_config_t SX126X_spi_config = {
			.mode           =  0,
			.spics_io_num   = SPI_SLAVE_SX1262_PIN,
			.clock_speed_hz =  1 * 1000 * 1000,
			.queue_size     =  1,
			.command_bits = 0,
			.address_bits = 6

		};

ret = spi_bus_add_device(SPI_BUS, &SX126X_spi_config, &spi_dev_handle_SX126X);
return ret;
}

sx126x_hal_status_t sx126x_hal_read(const void* context, const uint8_t *command, const uint16_t command_length,
                                     uint8_t *data, const uint16_t data_length) {

	//Transaction configuration//
	spi_transaction_ext_t trans;
	memset(&trans, 0x00, sizeof(trans));
	trans.base.flags = SPI_TRANS_VARIABLE_ADDR;
	trans.base.length = 8 * (command_length  + data_length);
	trans.base.rxlength = 8 * data_length;
	trans.address_bits = (command_length) * 8;
	trans.base.addr = command;
	trans.base.rx_buffer = data;

	//Transaction execution//
	spi_device_acquire_bus(spi_dev_handle_SX126X, portMAX_DELAY);
	if (spi_device_polling_transmit(spi_dev_handle_SX126X, (spi_transaction_t*) &trans) != ESP_OK)
	{
		ESP_LOGE("SPI DRIVER", "%s(%d): spi transmit failed", __FUNCTION__, __LINE__);

	}

	spi_device_release_bus(spi_dev_handle_SX126X);



	return 0;
}

sx126x_hal_status_t sx126x_hal_write(const void* context, const uint8_t *command, const uint16_t command_length,
									  const uint8_t *data, const uint16_t data_length) {

	//Transaction configuration//

	spi_transaction_ext_t trans;
	memset(&trans, 0x00, sizeof(trans));
	trans.base.flags = SPI_TRANS_VARIABLE_ADDR;
	trans.base.length = 8 * (command_length + data_length);
	trans.address_bits = command_length * 8;
	trans.base.addr = command;
	trans.base.tx_buffer = data;



	//Transaction execution//
	spi_device_acquire_bus(spi_dev_handle_SX126X, portMAX_DELAY);

	if (spi_device_polling_transmit(spi_dev_handle_SX126X, (spi_transaction_t*) &trans) != ESP_OK)

	 {
		ESP_LOGE("SPI DRIVER", "%s(%d): spi transmit failed", __FUNCTION__, __LINE__);
	}

	spi_device_release_bus(spi_dev_handle_SX126X);

	return 0;
}

sx126x_hal_status_t sx126x_hal_reset( const void* context ){
	vTaskDelay(pdMS_TO_TICKS(20));
	gpio_set_level(RF_RST_PIN, 0);
	vTaskDelay(pdMS_TO_TICKS(40));
	gpio_set_level(RF_RST_PIN, 1);
	vTaskDelay(20);

	return 0;
}

sx126x_hal_status_t sx126x_hal_wakeup( const void* context ){
	gpio_set_level(SPI_SLAVE_SX1262_PIN, 0);
	vTaskDelay(pdMS_TO_TICKS(2));
	gpio_set_level(SPI_SLAVE_SX1262_PIN, 1);
	vTaskDelay(pdMS_TO_TICKS(2));

	return 0;
}
