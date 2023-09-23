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
#include "esp_check.h"
#include "SPI_driver.h"
#include "sx126x_hal.h"

esp_err_t SX126X_spi_init(void);
uint32_t SX126X_getBUSY();
void SX126X_checkBusy();

static const char *TAG = "sx126x driver";
static spi_device_handle_t spi_dev_handle_SX126X;

esp_err_t SX126X_initIO(){
	ESP_RETURN_ON_ERROR(gpio_reset_pin(RF_BUSY_PIN), TAG, "Error settin RF_BUSY_PIN");
	ESP_RETURN_ON_ERROR(gpio_reset_pin(RF_RST_PIN),  TAG, "Error settin RF_RSTPIN");
	ESP_RETURN_ON_ERROR(gpio_set_direction(RF_BUSY_PIN, GPIO_MODE_INPUT),  TAG, "Error settin RF_BUSY_PIN");
	ESP_RETURN_ON_ERROR(gpio_set_direction(RF_RST_PIN,  GPIO_MODE_OUTPUT), TAG, "Error settin RF_RST_PIN");

	ESP_RETURN_ON_ERROR(SX126X_spi_init(),	 TAG, "SX126X_spi_init failed");
	sx126x_hal_reset(0);

	return ESP_OK;
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
	if(SPI_checkInit() != ESP_OK){
		ESP_LOGE(TAG, "SPI controller not initialized! Use SPI_init() in main.c");
		return ESP_ERR_INVALID_STATE;
	}

	/* CONFIGURE SPI DEVICE */
	/* Max SCK frequency - 16MHz */
	ESP_RETURN_ON_ERROR(SPI_registerDevice(&spi_dev_handle_SX126X, SPI_SLAVE_SX1262_PIN,
										SPI_SCK_16MHZ, 1, 0, 8), TAG, "SPI register failed");

	return ESP_OK;
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
