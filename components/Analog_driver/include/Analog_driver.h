#pragma once

typedef struct{
	uint8_t IGN1_det;
	uint8_t IGN2_det;
	uint8_t IGN3_det;
	uint8_t IGN4_det;
	uint32_t vbat_mV;
	float temp;
} Analog_meas_t;

esp_err_t Analog_init(uint32_t ign_det_thr_val);
uint32_t Analog_getIGN1();
uint32_t Analog_getIGN2();
uint32_t Analog_getIGN3();
uint32_t Analog_getIGN4();
uint32_t Analog_getVBAT();
float Analog_getTempMCU();
void Analog_update(Analog_meas_t *);
