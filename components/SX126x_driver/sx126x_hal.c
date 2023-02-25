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

esp_err_t SX126X_spi_init(void);
uint32_t SX126X_getBUSY();
void SX126X_checkBusy();

static const char *TAG = "sx126x driver";
static spi_device_handle_t spi_dev_handle_SX126X;

void SX126X_initIO(){
	gpio_reset_pin(RF_BUSY_PIN);
	gpio_reset_pin(RF_RST_PIN);

	gpio_set_direction(RF_BUSY_PIN, GPIO_MODE_INPUT);
	gpio_set_direction(RF_RST_PIN,  GPIO_MODE_OUTPUT);

	SX126X_spi_init();

	sx126x_hal_reset(0);
}

uint32_t SX126X_getBUSY() {
	return gpio_get_level(RF_BUSY_PIN);
}

void SX126X_checkBusy() {
	uint8_t busy_timeout_cnt;
	busy_timeout_cnt = 0;

	while (SX126X_getBUSY()) {
		vTaskDelay(1);
		busy_timeout_cnt++;
		if (busy_timeout_cnt > 5){ //wait 5mS for busy to complete
			busy_timeout_cnt = 0;
			//printf(F("ERROR - Busy Timeout!"));
			//resetDevice();          //reset device
			//setStandby(MODE_STDBY_RC);
			//config();               //re-run saved config
			break;
		}
	}
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
				.mode           = 0,
				.spics_io_num   = SPI_SLAVE_SX1262_PIN,
				.clock_speed_hz = 1 * 1000 * 1000,
				.queue_size     = 1,
				.command_bits 	= 0,
				.address_bits 	= 8
			};

	return spi_bus_add_device(SPI_BUS, &SX126X_spi_config, &spi_dev_handle_SX126X);
}

sx126x_hal_status_t sx126x_hal_read(const void* context, const uint8_t *command, const uint16_t command_length,
                                     uint8_t *data, const uint16_t data_length) {
	spi_transaction_ext_t trans;
	memset(&trans, 0x00, sizeof(trans));
	trans.base.flags  	 = SPI_TRANS_VARIABLE_ADDR;

	if(command_length <= 4){
		uint64_t cmd = 0;
		for(uint8_t i = 0;i<command_length;i++){
			cmd = cmd | ((uint64_t)(*(command+i)) << (8*((command_length-1)-i)));
		}
		//Transaction configuration//
		trans.base.length 	 = 8 * data_length;
		trans.base.rxlength  = 8 * data_length;
		trans.address_bits 	 = 8 * command_length;
		trans.base.addr 	 = cmd;
		trans.base.rx_buffer = data;
	}
	else {
		ESP_LOGE(TAG, "Unsuported SPI message  to SX1262, Cmd len = %i, Data len = %i", command_length, data_length);
	}

	SX126X_checkBusy();

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
	spi_transaction_ext_t trans;
	memset(&trans, 0x00, sizeof(trans));
	trans.base.flags  	 = SPI_TRANS_VARIABLE_ADDR;

	if(data_length == 0){	// No data, only CMD -> send cmd as data
		//Transaction configuration//
		trans.base.length 	 = 8 * command_length;
		trans.address_bits 	 = 0;
		trans.base.tx_buffer = command;
	}
	else if(command_length <= 4){
		uint64_t cmd = 0;
		for(uint8_t i = 0;i<command_length;i++){
			cmd = cmd | ((uint64_t)(*(command+i)) << (8*((command_length-1)-i)));
		}
		//Transaction configuration//
		trans.base.length 	 = 8 * data_length;
		trans.address_bits 	 = 8 * command_length;
		trans.base.addr 	 = cmd;
		trans.base.tx_buffer = data;
	}
	else {
		ESP_LOGE(TAG, "Unsuported SPI message  to SX1262, Cmd len = %i, Data len = %i", command_length, data_length);
	}

	SX126X_checkBusy();

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
