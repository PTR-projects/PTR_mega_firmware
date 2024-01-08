#pragma once
#include "esp_err.h"
#include "LIS331_driver.h"
#include "LSM6DSO32_driver.h"
#include "MMC5983MA_driver.h"
#include "MS5607_driver.h"

typedef struct{
	LIS331_meas_t 		LIS331;
	LSM6DS_meas_t 		LSM6DSO32[LSM6DSO32_COUNT];
	MMC5983MA_meas_t 	MMC5983MA;
	MS5607_meas_t 		MS5607;

	float ref_press;
} Sensors_t;

esp_err_t Sensors_init();
esp_err_t Sensors_update();
Sensors_t * Sensors_get();
esp_err_t Sensors_UpdateReferencePressure();
esp_err_t Sensors_calibrateGyro(float gain);
