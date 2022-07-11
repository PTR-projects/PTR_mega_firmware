#include <stdio.h>
#include "esp_err.h"
#include "Spi_driver.h"

#include "MS5607_driver.h"

#define slave SPI_SLAVE_MS5607

MS5607_cal_t MS5607_cal_d;

esp_err_t MS5607_init() {
	return ESP_OK;	//ESP_FAIL
}

esp_err_t MS5607_resetDevice() {

	SPI_RW(slave, MS5607_Reset, NULL, 2); //trzeba dokonczyc, co zwraca reset? I w razie failu wywalic blad

	return ESP_OK;
}

esp_err_t MS5607_readCalibration() {
	MS5607_resetDevice();

	uint8_t buf[17] = {0};
	SPI_RW(SPI_SLAVE_MS5607, MS5607_Prom_Read, buf, 16);

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

esp_err_t MS5607_startConv(char oversamplingRate) {
	SPI_RW(SPI_SLAVE_MS5607, oversamplingRate, NULL, 2); //Dodac errorhandler

	return ESP_OK;
}

esp_err_t MS5607_startMeas() {
	return ESP_OK;
}

void MS5607_readMeas(MS5607_t * data) {
	uint8_t[17] = {0};
	SPI_RW(SPI_SLAVE_MS5607, MS5607_Prom_Read, buf, 16);
}



