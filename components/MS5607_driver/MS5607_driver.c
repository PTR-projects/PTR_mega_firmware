#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "Spi_driver.h"

#include "MS5607_driver.h"

static void MS5607_read(uint8_t address, uint8_t * buf, uint8_t len);
static void MS5607_write(uint8_t address);
static esp_err_t MS5607_resetDevice();
static esp_err_t MS5607_readCalibration();
static esp_err_t MS5607_reqPress();
static esp_err_t MS5607_reqTemp();
static esp_err_t MS5607_readPress();
static esp_err_t MS5607_readTemp();
static esp_err_t  MS5607_calcPress();
static esp_err_t  MS5607_calcTemp();


static MS5607_cal_t MS5607_cal_d;
static MS5607_t 	MS5607_d;


esp_err_t MS5607_init() {
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

/**
 * @brief Reads data from the MS5607 sensor over SPI
 *
 * This function reads `len` bytes of data from the specified `address` on the
 * MS5607 sensor and stores the data in the `buf` buffer.
 *
 * @param[in] address The address to read from on the MS5607 sensor
 * @param[out] buf Pointer to the buffer where the read data will be stored
 * @param[in] len The number of bytes to read from the sensor
 */
static void MS5607_read(uint8_t address, uint8_t * buf, uint8_t len) {
    uint8_t txBuff[4] = {0U};
    txBuff[0] = address;
    SPI_RW(SPI_SLAVE_MS5607, txBuff, buf, len+1);
}

/**
 * @brief Writes data to the MS5607 sensor over SPI
 *
 * This function writes the specified `address` to the MS5607 sensor over SPI.
 *
 * @param[in] address The address to write to on the MS5607 sensor
 */
static void MS5607_write(uint8_t address) {
    uint8_t txBuff[1] = {address};
    SPI_RW(SPI_SLAVE_MS5607, txBuff, NULL, 1);
}

/**
 * @brief Resets the MS5607 sensor
 *
 * This function sends a reset command to the MS5607 sensor over SPI. It should
 * be called before initialization of the sensor.
 *
 * @return
 * - ESP_OK if the reset was successful
 * - ESP_FAIL if there was an error resetting the sensor
 */
static esp_err_t MS5607_resetDevice() {

	MS5607_write(MS5607_RESET);
	vTaskDelay(10/portTICK_PERIOD_MS);  //10ms
	return ESP_OK;
}

/**
 * @brief Reads the calibration data from the MS5607 sensor
 *
 * This function reads the calibration data from the MS5607 sensor and stores
 * it in the `MS5607_cal_t` structure. This data is used in the conversion
 * from raw readings to temperature and pressure readings.
 *
 * @return
 * - ESP_OK if the calibration data was successfully read
 * - ESP_FAIL if there was an error reading the calibration data
 */
static esp_err_t MS5607_readCalibration() {
	// Reset the sensor
	MS5607_resetDevice();

	// Read the calibration data
	uint8_t buf[3] = {0};
	MS5607_read(MS5607_PROM_READ, buf, 2);

	MS5607_read(MS5607_PROM_READ+2, buf, 2);
	MS5607_cal_d.C1 = ((uint16_t)buf[1] << 8) | buf[2];

	MS5607_read(MS5607_PROM_READ+4, buf, 2);
	MS5607_cal_d.C2 = ((uint16_t)buf[1] << 8) | buf[2];

	MS5607_read(MS5607_PROM_READ+6, buf, 2);
	MS5607_cal_d.C3 = ((uint16_t)buf[1] << 8) | buf[2];

	MS5607_read(MS5607_PROM_READ+8, buf, 2);
	MS5607_cal_d.C4 = ((uint16_t)buf[1] << 8) | buf[2];

	MS5607_read(MS5607_PROM_READ+10, buf, 2);
	MS5607_cal_d.C5 = ((uint16_t)buf[1] << 8) | buf[2];

	MS5607_read(MS5607_PROM_READ+12, buf, 2);
	MS5607_cal_d.C6 = ((uint16_t)buf[1] << 8) | buf[2];

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

/**
 * @brief Sends a command to the MS5607 sensor to start a pressure conversion
 *
 * This function sends a command to the MS5607 sensor over SPI to start a
 * pressure conversion. The result of the conversion can be read using
 * `MS5607_readPress()`.
 *
 * @return
 * - ESP_OK if the command was successfully sent
 * - ESP_FAIL if there was an error sending the command
 */
static esp_err_t MS5607_reqPress() {
	MS5607_write(MS5607_CONVERT_D1_256);
	return ESP_OK;
}

/**
 * @brief Sends a command to the MS5607 sensor to start a temperature conversion
 *
 * This function sends a command to the MS5607 sensor over SPI to start a
 * temperature conversion. The result of the conversion can be read using
 * `MS5607_readTemp()`.
 *
 * @return
 * - ESP_OK if the command was successfully sent
 * - ESP_FAIL if there was an error sending the command
 */
static esp_err_t MS5607_reqTemp() {
	MS5607_write(MS5607_CONVERT_D2_256);
	return ESP_OK;
}

/**
 * @brief Reads the result of a pressure conversion from the MS5607 sensor
 *
 * This function reads the result of a pressure conversion from the MS5607
 * sensor. `MS5607_reqPress()` must be called before this function in order
 * to start the conversion.
 *
 * @return
 * - ESP_OK if the result was successfully read
 * - ESP_FAIL if there was an error reading the result or if a conversion
 *   was not started before calling this function
 */
static esp_err_t MS5607_readPress() {
	uint8_t buf[4] = {0};
	MS5607_read(MS5607_ADC_READ, buf, 3);

	uint32_t D1 = ((uint32_t)(((uint32_t)buf[1])<<16 | ((uint32_t)buf[2])<<8 | (uint32_t)buf[3]));

	if(D1 == 0) {
		return ESP_FAIL; //Requested ADC_READ before conversion was completed
	}

	MS5607_d.D1 = D1;
	return ESP_OK;
}

/**
 * @brief Reads the result of a temperature conversion from the MS5607 sensor
 *
 * This function reads the result of a temperature conversion from the MS5607
 * sensor. `MS5607_reqTemp()` must be called before this function in order
 * to start the conversion.
 *
 * @return
 * - ESP_OK if the result was successfully read
 * - ESP_FAIL if there was an error reading the result or if a conversion
 *   was not started before calling this function
 */
static esp_err_t MS5607_readTemp() {
	uint8_t buf[4] = {0};
	MS5607_read(MS5607_ADC_READ, buf, 3);

	uint32_t D2 = ((uint32_t)(((uint32_t)buf[1])<<16 | ((uint32_t)buf[2])<<8 | (uint32_t)buf[3])) ;

	if(D2 == 0) {
		return ESP_FAIL; //Requested ADC_READ before conversion was completed
	}

	MS5607_d.D2 = D2;
	return ESP_OK;
}

/**
 * @brief Calculates the pressure from the raw data read from the MS5607 sensor
 *
 * This function calculates the pressure from the raw data read from the MS5607
 * sensor using the calibration data and stores the result in the `MS5607_d`
 * structure. `MS5607_readPress()` and `MS5607_readTemp()` must be called
 * before this function in order to read the raw data from the sensor.
 *
 * @return
 * - ESP_OK if the calculation was successful
 * - ESP_FAIL if there was an error in the calculation or if the raw data
 *   has not been read from the sensor
 */
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

/**
 * @brief Calculates the temperature from the raw data read from the MS5607 sensor
 *
 * This function calculates the temperature from the raw data read from the MS5607
 * sensor using the calibration data and stores the result in the `MS5607_d`
 * structure. `MS5607_readTemp()` must be called before this function in order
 * to read the raw data from the sensor.
 *
 * @return
 * - ESP_OK if the calculation was successful
 * - ESP_FAIL if there was an error in the calculation or if the raw data
 *   has not been read from the sensor
 */
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
