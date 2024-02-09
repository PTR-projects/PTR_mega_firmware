#pragma once

typedef struct{
	int8_t IGN_det[IGN_NUM];
	uint32_t vbat_mV;
	float temp;
} Analog_meas_t;

esp_err_t Analog_init(uint32_t ign_det_thr_val, float filter);
float Analog_getTempMCU();
void Analog_update(Analog_meas_t *);
int8_t Analog_getIGNstate(Analog_meas_t * meas, uint8_t ign_no);
