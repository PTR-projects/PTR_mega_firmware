#include <stdio.h>
#include <string.h>
#include "MS5607_driver.h"
#include "LIS331_driver.h"
#include "MMC5983MA_driver.h"
#include "LSM6DSO32_driver.h"

#include "Sensors.h"

esp_err_t Sensors_axes_translation();

//--------- Private var ---------------
static Sensors_t Sensors_d;

esp_err_t Sensors_init()
{
	memset(&Sensors_d, 0, sizeof(Sensors_d));

	MS5607_init();
//	LIS331_init(H3LIS331_IC_100G);
	MMC5983MA_init();
//	LSM6DSO32_init();

	return ESP_OK; 	//ESP_FAIL
}

esp_err_t Sensors_update(){
	//get new data from sensors

	MS5607_getReloadSmart();
//	LIS331_readMeas();
//	LSM6DSO32_readMeas();
	MMC5983MA_readMeas();

	MS5607_getMeas(&(Sensors_d.MS5607));
//	LIS331_getMeas(&(Sensors_d.LIS331));
//	LSM6DSO32_getMeas(&(Sensors_d.LSM6DSO32));
	MMC5983MA_getMeas(&(Sensors_d.MMC5983MA));

	Sensors_axes_translation();

	return ESP_OK; 	//ESP_FAIL
}

esp_err_t Sensors_axes_translation(){
	Sensors_t Sensors_b = Sensors_d;

	Sensors_d.LIS331.accY     = -Sensors_b.LIS331.accY;
	Sensors_d.LIS331.accZ     = -Sensors_b.LIS331.accZ;

	Sensors_d.LSM6DSO32.accX  =  Sensors_b.LSM6DSO32.accY;
	Sensors_d.LSM6DSO32.accY  =  Sensors_b.LSM6DSO32.accX;
	Sensors_d.LSM6DSO32.accZ  = -Sensors_b.LSM6DSO32.accZ;

	Sensors_d.LSM6DSO32.gyroX =  Sensors_b.LSM6DSO32.gyroY;
	Sensors_d.LSM6DSO32.gyroY =  Sensors_b.LSM6DSO32.gyroX;
	Sensors_d.LSM6DSO32.gyroZ = -Sensors_b.LSM6DSO32.gyroZ;

	Sensors_d.MMC5983MA.magX  = -Sensors_b.MMC5983MA.magY;
	Sensors_d.MMC5983MA.magY  = -Sensors_b.MMC5983MA.magX;
	Sensors_d.MMC5983MA.magZ  =  Sensors_b.MMC5983MA.magZ;

	return ESP_OK;
}

Sensors_t * Sensors_get(){
	return &Sensors_d;
}
