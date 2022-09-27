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

esp_err_t Sensors_init();
esp_err_t Sensors_update();
Sensors_t * Sensors_get();
