#pragma once
#include "esp_err.h"

typedef struct {
	uint32_t D1;
	uint32_t D2;
	int32_t dT;
	int64_t SENS2;
	int64_t OFF2;

	float temp;
	float press;
} MS5607_t;

typedef struct {
	uint16_t C1;
	uint16_t C2;
	uint16_t C3;
	uint16_t C4;
	uint16_t C5;
	uint16_t C6;
} MS5607_cal_t;



esp_err_t MS5607_init(MS5607_t * data);
esp_err_t MS5607_getReloadSmart(MS5607_t * data);



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
