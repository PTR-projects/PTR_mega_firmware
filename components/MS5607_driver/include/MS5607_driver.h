#pragma once
#include "esp_err.h"
#include "SPI_driver.h"

/**
 * @brief Barometer final measurement data
 */
typedef struct{
	float temp;		/*!< Temperature */
	float press;	/*!< Pressure */
} MS5607_meas_t;

/**
 * @brief Barometer calibration constants
 */
typedef struct {
	uint16_t C1;		/*!< Pressure sensitivity */
	uint16_t C2;		/*!< Pressure offset */
	uint16_t C3;		/*!< Temperature coefficient of pressure sensitivity */
	uint16_t C4;		/*!< Temperature coefficient of pressure offset */
	uint16_t C5;		/*!< Reference temperature */
	uint16_t C6;		/*!< Temperature coefficient of the temperature */
} MS5607_cal_t;

/**
 * @brief Barometer data
 */
typedef struct {
	uint32_t D1;		/*!< Digital pressure value */
	uint32_t D2;		/*!< Digital temperature value */
	int32_t dT;			/*!< Difference between actual and reference temperature */
	int64_t SENS2;		/*!< Sensitivity at actual temperature */
	int64_t OFF2;		/*!< Offset at actual temperature */

	MS5607_meas_t meas;
} MS5607_t;

typedef struct {
	spi_dev_handle_t spi_handle;
	MS5607_cal_t calibration;

	uint32_t D1;
	uint32_t D2;
	int32_t dT;
	int64_t SENS2;
	int64_t OFF2;

	MS5607_meas_t meas;
} MS5607_t;

/**
 * @brief Initialize MS5607 barometer sensor
 *
 * @return esp_err_t
 *  - ESP_OK: Success
 *  - ESP_FAIL: Fail
 */
esp_err_t MS5607_init();

/**
 * @brief Perform sensor data read with different data rates for temperature and pressure
 *
 * @return esp_err_t
 *  - ESP_OK: Success
 *	- ESP_FAIL: Fail
 */
esp_err_t MS5607_getReloadSmart();

/**
 * @brief Get computed pressure 
 *
 * @return float
 * - Pressure value in Pascals
 */
float MS5607_getPress();

/**
 * @brief Get computed temperature 
 *
 * @return float
 * - Pressure value in degrees Celcius
 */
float MS5607_getTemp();

/**
 * @brief Get computed pressure and temperature written into pointed struct 
 *
 * @param sensor specifies sensor number
 * @param meas struct to wchich we need to write data
 * @return esp_err_t
 *  - ESP_OK: Success
 *	- ESP_FAIL: Fail
 */
esp_err_t MS5607_getMeas(uint8_t sensor, MS5607_meas_t * meas);


/*
 * Registers
 */
#define MS5607_ADC_READ 		0x00
#define MS5607_PROM_READ		0xA0
#define MS5607_RESET			0x1E


/*
 * Digital pressure oversampling settings
 */
#define MS5607_CONVERT_D1_256 	0x40
#define MS5607_CONVERT_D1_512 	0x42
#define MS5607_CONVERT_D1_1024 	0x44
#define MS5607_CONVERT_D1_2048 	0x46
#define MS5607_CONVERT_D1_4096	0x48


/*
 * Digital temperature oversampling settings
 */
#define MS5607_CONVERT_D2_256 	0x50
#define MS5607_CONVERT_D2_512 	0x52
#define MS5607_CONVERT_D2_1024 	0x54
#define MS5607_CONVERT_D2_2048 	0x56
#define MS5607_CONVERT_D2_4096	0x58
