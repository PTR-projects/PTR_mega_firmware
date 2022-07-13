#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "Spi_driver.h"

#include "MS5607_driver.h"


MS5607_cal_t MS5607_cal_d;

esp_err_t MS5607_readCalibration() {
	MS5607_resetDevice();

	uint8_t buf[16] = {0};
	MS5607_read(MS5607_PROM_READ, buf, 17);

	MS5607_cal_d.C1 = *((uint16_t*)&buf[2]);
	MS5607_cal_d.C2 = *((uint16_t*)&buf[4]);
	MS5607_cal_d.C3 = *((uint16_t*)&buf[6]);
	MS5607_cal_d.C4 = *((uint16_t*)&buf[8]);
	MS5607_cal_d.C5 = *((uint16_t*)&buf[10]);
	MS5607_cal_d.C6 = *((uint16_t*)&buf[12]);


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

void MS5607_read(uint8_t address, uint8_t * buf, uint8_t len) {
    uint8_t txBuff[4] = {0U};
    txBuff[0] = address;
    SPI_RW(SPI_SLAVE_MS5607, txBuff, buf, len+1);
}

void MS5607_write(uint8_t address) {
    uint8_t txBuff[1] = {address};
    SPI_RW(SPI_SLAVE_MS5607, txBuff, NULL, 1);
}

esp_err_t MS5607_resetDevice() {

	MS5607_write(MS5607_RESET);
	vTaskDelay(4/portTICK_PERIOD_MS);  //4ms/1ms = 4 ticks
	return ESP_OK;
}

esp_err_t MS5607_reqPress() {
	MS5607_write(MS5607_CONVERT_D1_256);
	return ESP_OK;
}

esp_err_t MS5607_reqTemp() {
	MS5607_write(MS5607_CONVERT_D2_256);
	return ESP_OK;
}

esp_err_t MS5607_getPress(MS5607_t * data) {
	uint8_t buf[3] = {0};
	MS5607_read(MS5607_ADC_READ, buf, 4);
	data -> D1 = ((uint32_t)(((uint32_t)buf[0])<<16 | ((uint32_t)buf[1])<<8 | (uint32_t)buf[2])) >> 4;

	if(data->D1 == 0) {
		return ESP_FAIL; //Requested ADC_READ before conversion was completed
	}
	return ESP_OK;
}

esp_err_t MS5607_getTemp(MS5607_t * data) {
	uint8_t buf[3] = {0};
	MS5607_read(MS5607_ADC_READ, buf, 4);

	data->D2 = ((uint32_t)(((uint32_t)buf[0])<<16 | ((uint32_t)buf[1])<<8 | (uint32_t)buf[2])) >> 4;

	if(data->D2 == 0) {
		return ESP_FAIL; //Requested ADC_READ before conversion was completed
	}
	return ESP_OK;
}

esp_err_t  MS5607_calcPress(MS5607_t * data) {
	int64_t OFF = (((int64_t)MS5607_cal_d.C2) * ((int64_t)1<<17)) + ((((int64_t)MS5607_cal_d.C4) * ((int64_t)(data->dT))) / ((int64_t)1<<6)) - data->OFF2;
	int64_t SENS = (((int64_t)MS5607_cal_d.C1) * ((int64_t)1<<16)) + (((int64_t)MS5607_cal_d.C3) * ((int64_t)data->dT)) /  ((int64_t)1<<7) - data->SENS2;
	int64_t P =  (((((int64_t)data->D1) * SENS) / ((int64_t)1<<21)) - OFF) / ((int64_t)1<<15);
	data->press = ((float)P) / 100.0;

	return ESP_OK;
}

esp_err_t  MS5607_calcTemp(MS5607_t * data) {
	int32_t dT =  ((int64_t)data -> D2) - (((int64_t)MS5607_cal_d.C5) *  ((int64_t)1<<8));
	int32_t TEMP = 2000 + dT * (((int64_t)MS5607_cal_d.C6) / ((int64_t)1<<23));

	/*
	 * Second order temperature compensation (as per datasheet)
	 */
	int32_t T2;
	int32_t OFF2;
	int64_t SENS2;
	if(TEMP < 2000) {
		T2 = dT / ((uint32_t)1<<31);
		OFF2 = (61 * ((TEMP-2000) * (TEMP-2000))) / ((uint32_t)1<<4);
		SENS2 = 2 * ((((uint64_t)TEMP)-2000) * (((uint64_t)TEMP)-2000));

		if(TEMP < -1500) {
			OFF2 = OFF2 + (15 * ((TEMP + 1500) * (TEMP + 1500)));
			SENS2 = SENS2 + (8 * ((TEMP + 1500) * (TEMP + 1500)));
		}
	}
	else {
		T2 = 0;
		OFF2 = 0;
		SENS2 = 0;
	}

	TEMP = TEMP - T2;
	data->OFF2 = OFF2;
	data->SENS2 = SENS2;

	data->temp = ((float)TEMP) / 100.0;


	return ESP_OK;
}


void MS5607_getReloadSmart(MS5607_t * data){
    static int8_t count = -1;
    count++;

    if((count != 0 ) && (count < 100)){
        MS5607_getPress(data);
        MS5607_reqPress();
    }

    if(count == 100){
        MS5607_getPress(data);
        MS5607_reqTemp();
    }

    if(count == 101){
        count = 1;
        MS5607_getTemp(data);
        MS5607_reqPress();
    }

    if(count == 0){
        MS5607_reqPress();
        vTaskDelay(12/portTICK_PERIOD_MS);  //12ms/1ms = 12 ticks
        MS5607_getPress(data);
        MS5607_reqTemp();
        vTaskDelay(12/portTICK_PERIOD_MS);  //12ms/1ms = 12 ticks
        MS5607_getTemp(data);
        MS5607_reqPress();
    }

    data->temp = count;
    MS5607_calcTemp(data);
    MS5607_calcPress(data);
}

esp_err_t MS5607_init() {
	MS5607_readCalibration();

	return ESP_OK;
}



