#pragma once
#include "esp_err.h"
#include "LIS331_driver.h"
#include "LSM6DSO32_driver.h"
#include "MMC5983MA_driver.h"
#include "MS5607_driver.h"

typedef struct{
	LIS331_meas_t 		LIS331;
	LSM6DS_meas_t 		LSM6DSO32;
	MMC5983MA_meas_t 	MMC5983MA;
	MS5607_meas_t 		MS5607;
} Sensors_t;

/**
 * @brief Initialize all the sensors present
 *
 * @return esp_err_t
 *  - ESP_OK: Success
 *	- ESP_FAIL: Other errors
 */
esp_err_t Sensors_init();

/**
 * @brief Update all the present sensors and perform exes translation 
 *
 * @return esp_err_t
 *  - ESP_OK: Success
 *	- ESP_FAIL: Other errors
 */
esp_err_t Sensors_update();

/**
 * @brief Extract sensor data from the component
 *
 * @return Sensors_t
 */
Sensors_t * Sensors_get();
 