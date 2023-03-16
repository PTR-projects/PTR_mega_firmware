#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_check.h"
#include "SPI_driver.h"
#include "esp_log.h"
#include "MS5607_driver.h"
#include "BOARD.h"
#include <string.h>

static const char *TAG = "MS5607";

static esp_err_t MS5607_read(uint8_t addr, uint8_t * data_in, uint16_t length);
static esp_err_t MS5607_write(uint8_t addr);
static esp_err_t MS5607_resetDevice();
static esp_err_t MS5607_readCalibration();
static esp_err_t MS5607_reqPress();
static esp_err_t MS5607_reqTemp();
static esp_err_t MS5607_readPress();
static esp_err_t MS5607_readTemp();
static esp_err_t MS5607_calcPress();
static esp_err_t MS5607_calcTemp();


static MS5607_cal_t 	MS5607_cal_d;
static MS5607_t 		MS5607_d;
static spi_dev_handle_t spi_dev_handle_MS5607;

esp_err_t MS5607_spi_init(void)
{
	if(SPI_checkInit() != ESP_OK){
		ESP_LOGE(TAG, "SPI controller not initialized! Use SPI_init() in main.c");
		return ESP_ERR_INVALID_STATE;
	}

	/* CONFIGURE SPI DEVICE */
	ESP_RETURN_ON_ERROR(SPI_registerDevice(&spi_dev_handle_MS5607, SPI_SLAVE_MS5607_PIN, 1, 1, 0, 8), TAG, "SPI register failed");

	return ESP_OK;
}

esp_err_t MS5607_init() {
	MS5607_spi_init();
	MS5607_resetDevice();
	MS5607_readCalibration();

	MS5607_reqPress();
	vTaskDelay(12/portTICK_PERIOD_MS);  //12ms/1ms = 12 ticks
	MS5607_readPress();
	MS5607_reqTemp();
	vTaskDelay(12/portTICK_PERIOD_MS);  //12ms/1ms = 12 ticks
	MS5607_readTemp();
	MS5607_reqPress();

	return ESP_OK;
}

esp_err_t MS5607_getReloadSmart(){
    static int8_t count = -1;
    count++;

    if((count != 0 ) && (count < 100)){
        MS5607_readPress();
        MS5607_reqPress();
    }

    if(count == 100){
        MS5607_readPress();
        MS5607_reqTemp();
    }

    if(count == 101){
        count = 1;
        MS5607_readTemp();
        MS5607_reqPress();
    }

    if(count == 0){
        MS5607_reqPress();
    }

    MS5607_calcTemp();
    MS5607_calcPress();

    return ESP_OK;
}

float MS5607_getPress(){
	return MS5607_d.meas.press;
}

float MS5607_getTemp(){
	return MS5607_d.meas.temp;
}

esp_err_t MS5607_getMeas(MS5607_meas_t * meas){
	*meas = MS5607_d.meas;

	return ESP_OK;
}

static esp_err_t MS5607_read(uint8_t addr, uint8_t * data_in, uint16_t length) {
	return SPI_transfer(spi_dev_handle_MS5607, 0, addr, NULL, data_in, length);
}

static esp_err_t MS5607_write(uint8_t addr) {
	return SPI_transfer(spi_dev_handle_MS5607, 0, addr, NULL, NULL, 0);	// możliwe, że len = 8
}

static esp_err_t MS5607_resetDevice() {

	MS5607_write(MS5607_RESET);
	vTaskDelay(10/portTICK_PERIOD_MS);  //10ms
	return ESP_OK;
}

static esp_err_t MS5607_readCalibration() {
	MS5607_resetDevice();

	uint8_t buf[2] = {0};
	MS5607_read(MS5607_PROM_READ, buf, 1);

	MS5607_read(MS5607_PROM_READ+2, buf, 2);
	MS5607_cal_d.C1 = ((uint16_t)buf[0] << 8) | buf[1];

	MS5607_read(MS5607_PROM_READ+4, buf, 2);
	MS5607_cal_d.C2 = ((uint16_t)buf[0] << 8) | buf[1];

	MS5607_read(MS5607_PROM_READ+6, buf, 2);
	MS5607_cal_d.C3 = ((uint16_t)buf[0] << 8) | buf[1];

	MS5607_read(MS5607_PROM_READ+8, buf, 2);
	MS5607_cal_d.C4 = ((uint16_t)buf[0] << 8) | buf[1];

	MS5607_read(MS5607_PROM_READ+10, buf, 2);
	MS5607_cal_d.C5 = ((uint16_t)buf[0] << 8) | buf[1];

	MS5607_read(MS5607_PROM_READ+12, buf, 2);
	MS5607_cal_d.C6 = ((uint16_t)buf[0] << 8) | buf[1];

	return ESP_OK;


	/*
	 * 0 - > factory data
	 * 1 - > C1
	 * 2 - > C2
	 * 3 - > C3
	 * 4 - > C4
	 * 5 - > C5
	 * 6 - > C6
	 * 7 - > Serial code + CRC
	 *
	 * 8 bytes - > 128 bits
	 */
}

static esp_err_t MS5607_reqPress() {
	MS5607_write(MS5607_CONVERT_D1_256);
	return ESP_OK;
}

static esp_err_t MS5607_reqTemp() {
	MS5607_write(MS5607_CONVERT_D2_256);
	return ESP_OK;
}

static esp_err_t MS5607_readPress() {
	uint8_t buf[3] = {0};
	MS5607_read(MS5607_ADC_READ, buf, 3);

	uint32_t D1 = ((uint32_t)(((uint32_t)buf[0])<<16 | ((uint32_t)buf[1])<<8 | (uint32_t)buf[2]));

	if(D1 == 0) {
		return ESP_FAIL; //Requested ADC_READ before conversion was completed
	}

	MS5607_d.D1 = D1;
	return ESP_OK;
}

static esp_err_t MS5607_readTemp() {
	uint8_t buf[3] = {0};
	MS5607_read(MS5607_ADC_READ, buf, 3);

	uint32_t D2 = ((uint32_t)(((uint32_t)buf[0])<<16 | ((uint32_t)buf[1])<<8 | (uint32_t)buf[2])) ;

	if(D2 == 0) {
		return ESP_FAIL; //Requested ADC_READ before conversion was completed
	}

	MS5607_d.D2 = D2;
	return ESP_OK;
}

static esp_err_t  MS5607_calcPress() {
	int64_t dT = MS5607_d.dT;
	int64_t C1 = MS5607_cal_d.C1;
	int64_t C2 = MS5607_cal_d.C2;
	int64_t C3 = MS5607_cal_d.C3;
	int64_t C4 = MS5607_cal_d.C4;
	int64_t D1 = MS5607_d.D1;

	int64_t OFF  = (C2 << 17) + ((C4 * dT) >> 6);
	int64_t SENS = (C1 << 16) + ((C3 * dT) >> 7);
	int64_t P    = (((D1 * SENS) >> 21) - OFF) >> 15;
	MS5607_d.meas.press  = (float)P;

	return ESP_OK;
}

static esp_err_t  MS5607_calcTemp() {
	uint64_t C5 = MS5607_cal_d.C5;
	int64_t  C6 = MS5607_cal_d.C6;
	int64_t  D2 = MS5607_d.D2;

	int32_t dT   = D2 - (C5 << 8);
	int32_t TEMP = 2000 + ((dT * C6) >> 23);

//	---------------------- Do dopracowania ----------------------------------
//	/*
//	 * Second order temperature compensation (as per datasheet)
//	 */
//	int32_t T2;
//	int32_t OFF2;
//	int64_t SENS2;
//	if(TEMP < 2000) {
//		T2 = dT / ((uint32_t)1<<31);
//		OFF2 = (61 * ((TEMP-2000) * (TEMP-2000))) / ((uint32_t)1<<4);
//		SENS2 = 2 * ((((uint64_t)TEMP)-2000) * (((uint64_t)TEMP)-2000));
//
//		if(TEMP < -1500) {
//			OFF2 = OFF2 + (15 * ((TEMP + 1500) * (TEMP + 1500)));
//			SENS2 = SENS2 + (8 * ((TEMP + 1500) * (TEMP + 1500)));
//		}
//	}
//	else {
//		T2 = 0;
//		OFF2 = 0;
//		SENS2 = 0;
//	}
//
//	TEMP = TEMP - T2;
//	data->OFF2 = OFF2;
//	data->SENS2 = SENS2;

	MS5607_d.dT   = dT;
	MS5607_d.meas.temp = ((float)TEMP) / 100.0f;

	return ESP_OK;
}
