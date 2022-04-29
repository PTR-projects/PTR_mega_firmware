#pragma once

#include "../../LIS331_driver/include/LIS331_driver.h"
#include "../../LSM6DS032_driver/include/LSM6DS032_driver.h"
#include "../../MMC5983_driver/include/MMC5983_driver.h"
#include "../../MS5607_driver/include/MS5607_driver.h"


typedef struct{
	LIS331_t 	LIS331;
	LSM6DS032_t LSM6DS032;
	MMC5983_t 	MMC5983;
	MS5607_t 	MS5607;
} Sensors_t;

esp_err_t Sensors_init();
esp_err_t Sensors_update();
