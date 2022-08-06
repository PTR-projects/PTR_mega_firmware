/*
 * SX1262_hal.c
 *
 *  Created on: 22 maj 2019
 *      Author: b.moczala
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include <string.h>
#include "BOARD.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "SPI_driver.h"
#include "sx126x_hal.h"

void SX126X_initIO(){
	gpio_reset_pin(RF_BUSY_PIN);
	gpio_reset_pin(RF_RST_PIN);

	gpio_set_direction(RF_BUSY_PIN, GPIO_MODE_INPUT);
	gpio_set_direction(RF_RST_PIN,  GPIO_MODE_OUTPUT);

	sx126x_hal_reset(0);
}

void SX126X_SPICS(uint8_t value){
	SPI_CS(SPI_SLAVE_SX1262, value);
}

void SX126X_SPItransfer(uint8_t * buf_out, uint8_t * buf_in, uint16_t length){
	SPI_RW(SPI_SLAVE_NONE, buf_out, buf_in, length);
}

uint32_t SX126X_getBUSY() {
	return gpio_get_level(RF_BUSY_PIN);
}

void SX126X_checkBusy() {
	uint8_t busy_timeout_cnt;
	busy_timeout_cnt = 0;

	while (SX126X_getBUSY()) {
		vTaskDelay(2);
		busy_timeout_cnt++;

		if (busy_timeout_cnt > 10){ //wait 5mS for busy to complete
			busy_timeout_cnt = 0;
			//printf(F("ERROR - Busy Timeout!"));
			//resetDevice();          //reset device
			//setStandby(MODE_STDBY_RC);
			//config();               //re-run saved config
			break;
		}
	}
}

void SX126X_writeCommand(uint8_t Opcode, uint8_t *buffer, uint16_t size) {
	SX126X_checkBusy();

	uint8_t buf_out[1] = { Opcode };

	SX126X_SPICS(0);
	SX126X_SPItransfer(buf_out, buf_out, 1);
	SX126X_SPItransfer(buffer, buffer, size);
	SX126X_SPICS(1);

	if (Opcode != 0x84) {
		SX126X_checkBusy();
	}
}


sx126x_hal_status_t sx126x_hal_read(const void* context, const uint8_t *command, const uint16_t command_length,
                                     uint8_t *data, const uint16_t data_length) {
	SX126X_checkBusy();

	memset(data, 0xFF, data_length);

	SX126X_SPICS(0);
	SX126X_SPItransfer(command, NULL, command_length);
	SX126X_SPItransfer(data, data, data_length);
	SX126X_SPICS(1);

	return 0;
}
 
sx126x_hal_status_t sx126x_hal_write(const void* context, const uint8_t *command, const uint16_t command_length,
									  const uint8_t *data, const uint16_t data_length) {
	SX126X_checkBusy();

	memset(data, 0xFF, data_length);

	SX126X_SPICS(0);
	SX126X_SPItransfer(command, NULL, command_length);
	SX126X_SPItransfer(data, NULL, data_length);
	SX126X_SPICS(1);

	return 0;
}		

sx126x_hal_status_t sx126x_hal_reset( const void* context ){
	vTaskDelay(20);
	gpio_set_level(RF_RST_PIN, 0);
	vTaskDelay(40);
	gpio_set_level(RF_RST_PIN, 1);
	vTaskDelay(20);

	return 0;
}			  

sx126x_hal_status_t sx126x_hal_wakeup( const void* context ){
	SX126X_SPICS(0);
	vTaskDelay(2);
	SX126X_SPICS(1);
	vTaskDelay(2);

	return 0;
}







