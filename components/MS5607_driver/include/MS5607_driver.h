#pragma once
#include "esp_err.h"

typedef struct {
	uint32_t D1;
	uint32_t D2;

	int32_t temp_raw;
	int32_t pres_raw;
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


esp_err_t MS5607_init();
esp_err_t MS5607_resetDevice();
esp_err_t MS5607_startConv(char oversamplingRate);
esp_err_t MS5607_startMeas();
void MS5607_readMeas(MS5607_t * data);



/*
 * Registers
 */
#define MS5607_ADC_Read 		0x00
#define MS5607_Prom_Read		0xA0
#define MS5607_Reset 			0x1E


/*
 * Digital pressure oversampling settings
 */
#define MS5607_Convert_D1_256 	0x40
#define MS5607_Convert_D1_512 	0x42
#define MS5607_Convert_D1_1024 	0x44
#define MS5607_Convert_D1_2048 	0x46
#define MS5607_Convert_D1_4096	0x48


/*
 * Digital temperature oversampling settings
 */
#define MS5607_Convert_D2_256 	0x50
#define MS5607_Convert_D2_512 	0x52
#define MS5607_Convert_D2_1024 	0x54
#define MS5607_Convert_D2_2048 	0x56
#define MS5607_Convert_D2_4096	0x58
