#pragma once
#include "esp_err.h"

typedef struct{
	float accX;
	float accY;
	float accZ;
} LIS331_meas_t;

typedef struct {
	union{
		uint8_t raw[10];
		struct{
			int16_t empty;		//0 1
			int16_t accX_raw;	//2 3
			int16_t accY_raw;	//4 5
			int16_t accZ_raw;	//6 7
		};
	};

	LIS331_meas_t meas;

	float accXoffset;
	float accYoffset;
	float accZoffset;

	float sensor_range;
} LIS331_t;

typedef enum{
	LIS331_IC_2G 		= 2,
	LIS331_IC_4G 		= 4,
	LIS331_IC_8G 		= 8,
	LIS331HH_IC_6G 		= 6,
	LIS331HH_IC_12G 	= 12,
	LIS331HH_IC_24G 	= 24,
	H3LIS331_IC_100G 	= 100,
	H3LIS331_IC_200G 	= 200,
	H3LIS331_IC_400G 	= 400
} LIS331_type_t;

esp_err_t LIS331_init(LIS331_type_t type);
