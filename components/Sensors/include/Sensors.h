#pragma once
#include "esp_err.h"
#include "LIS331_driver.h"
#include "LSM6DSO32_driver.h"
#include "MMC5983MA_driver.h"
#include "MS5607_driver.h"

/**
 * @brief Structure to hold the measured data from multiple sensors.
 */
typedef struct{
	LIS331_meas_t 		LIS331;		/*!< Structure containing the measured data from a LIS331 accelerometer. */
	LSM6DS_meas_t 		LSM6DSO32;	/*!< Structure containing the measured data from a LSM6DSO32 accelerometer and gyroscope. */
	MMC5983MA_meas_t 	MMC5983MA;	/*!< Structure containing the measured data from a MMC5983MA magnetometer. */
	MS5607_meas_t 		MS5607;		/*!< Structure containing the measured data from a MS5607 barometer. */
} Sensors_t;

/**
 * @brief Initializes the sensors component.
 * @return ESP_OK if initialization was successful, ESP_FAIL otherwise.
 */
esp_err_t Sensors_init();

/**
 * @brief Updates the measurements for all sensors.
 * @return ESP_OK if update was successful, ESP_FAIL otherwise.
 */
esp_err_t Sensors_update();

/**
 * @brief Returns a pointer to the current sensor measurements.
 * @return A pointer to the current sensor measurements.
 */
Sensors_t * Sensors_get();
