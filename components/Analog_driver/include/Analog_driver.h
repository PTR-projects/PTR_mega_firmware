#pragma once

typedef struct{
	int8_t IGN1_det;
	int8_t IGN2_det;
	int8_t IGN3_det;
	int8_t IGN4_det;
	uint32_t vbat_mV;
	float temp;
} Analog_meas_t;

esp_err_t Analog_init(uint32_t ign_det_thr_val, float filter);
float Analog_getTempMCU();
void Analog_update(Analog_meas_t *);
