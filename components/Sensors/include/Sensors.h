#pragma once
#include "esp_err.h"
#include "LIS331_driver.h"
#include "LSM6DSO32_driver.h"
#include "MMC5983MA_driver.h"
#include "MS5607_driver.h"

typedef struct{
	LIS331_t 	LIS331;
	LSM6DSO32_t LSM6DSO32;
	MMC5983MA_t MMC5983MA;
	MS5607_t 	MS5607;
} Sensors_t;

esp_err_t Sensors_init();
esp_err_t Sensors_update();
