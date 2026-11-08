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
#include "BOARD_cfg.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_check.h"
#include "SPI_driver.h"
#include "sx126x_hal.h"

static const char *TAG = "sx126x driver";

#if !(defined (RF_BUSY_PIN) && defined (RF_RST_PIN) && defined (SPI_SLAVE_SX1262_PIN))
esp_err_t SX126X_initIO() {return ESP_OK;}
void SX126X_checkBusy() {}

#else
static spi_device_handle_t spi_dev_handle_SX126X;
esp_err_t SX126X_spi_init(void);
uint32_t SX126X_getBUSY();
static bool SX126X_waitBusyClear(void);

/* Full-duplex scratch (SPI bus held; not re-entrant). */
#define SX126X_HAL_XFER_MAX 260
static uint8_t sx126x_hal_tx[SX126X_HAL_XFER_MAX];
static uint8_t sx126x_hal_rx[SX126X_HAL_XFER_MAX];

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

static bool SX126X_waitBusyClear(void) {
	uint32_t busy_timeout_cnt = 0;

	while (SX126X_getBUSY()) {
		vTaskDelay(1);
		busy_timeout_cnt++;
		/* BUSY can stay high for tens of ms; SPI while busy corrupts status reads. */
		if (busy_timeout_cnt > 100) {
			ESP_LOGW(TAG, "SX126x BUSY timeout after %lums", (unsigned long)busy_timeout_cnt);
			return false;
		}
	}
	return true;
}

void SX126X_checkBusy() {
	(void)SX126X_waitBusyClear();
}

esp_err_t SX126X_spi_init(void)
{
	if(SPI_checkInit() != ESP_OK){
		ESP_LOGE(TAG, "SPI controller not initialized! Use SPI_init() in main.c");
		return ESP_ERR_INVALID_STATE;
	}

	/* Pure buffer transfers: cmd/addr bits must be 0 so frames are not shifted.
	 * 16 MHz intermittently inserted 0xFF before LEN (seen as len=255,ptr=14). */
	ESP_RETURN_ON_ERROR(SPI_registerDevice(&spi_dev_handle_SX126X, SPI_SLAVE_SX1262_PIN,
										SPI_SCK_8MHZ, 1, 0, 0), TAG, "SPI register failed");

	return ESP_OK;
}

sx126x_hal_status_t sx126x_hal_read(const void* context, const uint8_t *command, const uint16_t command_length,
                                     uint8_t *data, const uint16_t data_length) {
	(void)context;
	const uint16_t total = (uint16_t)(command_length + data_length);
	if(total == 0 || total > SX126X_HAL_XFER_MAX || command == NULL || (data_length && data == NULL)) {
		ESP_LOGE(TAG, "SX126x HAL read bad len cmd=%u data=%u", command_length, data_length);
		return SX126X_HAL_STATUS_ERROR;
	}

	if(!SX126X_waitBusyClear()) {
		return SX126X_HAL_STATUS_ERROR;
	}

	/* RadioLib/Semtech full-duplex: MOSI = CMD[+NOPs] + NOP padding; MISO aligned. */
	memcpy(sx126x_hal_tx, command, command_length);
	memset(sx126x_hal_tx + command_length, 0x00, data_length);
	memset(sx126x_hal_rx, 0x00, total);

	spi_transaction_t trans;
	memset(&trans, 0x00, sizeof(trans));
	trans.length    = 8 * total;
	trans.rxlength  = 8 * total;
	trans.tx_buffer = sx126x_hal_tx;
	trans.rx_buffer = sx126x_hal_rx;

	spi_device_acquire_bus(spi_dev_handle_SX126X, portMAX_DELAY);
	esp_err_t err = spi_device_polling_transmit(spi_dev_handle_SX126X, &trans);
	spi_device_release_bus(spi_dev_handle_SX126X);
	if(err != ESP_OK) {
		ESP_LOGE("SPI DRIVER", "%s(%d): spi transmit failed", __FUNCTION__, __LINE__);
		return SX126X_HAL_STATUS_ERROR;
	}

	memcpy(data, sx126x_hal_rx + command_length, data_length);

	return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_write(const void* context, const uint8_t *command, const uint16_t command_length,
									  const uint8_t *data, const uint16_t data_length) {
	(void)context;
	const uint16_t total = (uint16_t)(command_length + data_length);
	if(total == 0 || total > SX126X_HAL_XFER_MAX || command == NULL || (data_length && data == NULL)) {
		ESP_LOGE(TAG, "SX126x HAL write bad len cmd=%u data=%u", command_length, data_length);
		return SX126X_HAL_STATUS_ERROR;
	}

	if(!SX126X_waitBusyClear()) {
		return SX126X_HAL_STATUS_ERROR;
	}

	memcpy(sx126x_hal_tx, command, command_length);
	if(data_length) {
		memcpy(sx126x_hal_tx + command_length, data, data_length);
	}

	spi_transaction_t trans;
	memset(&trans, 0x00, sizeof(trans));
	trans.length    = 8 * total;
	trans.tx_buffer = sx126x_hal_tx;
	trans.rx_buffer = NULL;

	spi_device_acquire_bus(spi_dev_handle_SX126X, portMAX_DELAY);
	esp_err_t err = spi_device_polling_transmit(spi_dev_handle_SX126X, &trans);
	spi_device_release_bus(spi_dev_handle_SX126X);
	if(err != ESP_OK) {
		ESP_LOGE("SPI DRIVER", "%s(%d): spi transmit failed", __FUNCTION__, __LINE__);
		return SX126X_HAL_STATUS_ERROR;
	}

	return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_reset( const void* context ){
	(void)context;
	vTaskDelay(pdMS_TO_TICKS(20));
	gpio_set_level(RF_RST_PIN, 0);
	vTaskDelay(pdMS_TO_TICKS(40));
	gpio_set_level(RF_RST_PIN, 1);
	vTaskDelay(20);

	return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_wakeup( const void* context ){
	(void)context;
	gpio_set_level(SPI_SLAVE_SX1262_PIN, 0);
	vTaskDelay(pdMS_TO_TICKS(2));
	gpio_set_level(SPI_SLAVE_SX1262_PIN, 1);
	vTaskDelay(pdMS_TO_TICKS(2));

	return SX126X_HAL_STATUS_OK;
}
#endif
