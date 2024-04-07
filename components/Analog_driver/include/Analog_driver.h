#pragma once

/**
 * @brief Data structure representing analog measurements.
 */
typedef struct{
	int8_t IGN1_det;
	int8_t IGN2_det;
	int8_t IGN3_det;
	int8_t IGN4_det;
	uint32_t vbat_mV;
	float temp;
} Analog_meas_t;

/**
 * @brief Initialize the analog measurement module.
 * @param ign_det_thr_val Threshold value for IGN detection.
 * @param filter Filter constant for the temperature measurement.
 * @return ESP_OK if initialization was successful, ESP_FAIL otherwise.
 */
esp_err_t Analog_init(uint32_t ign_det_thr_val, float filter);

/**
 * @brief Get the temperature of the microcontroller (MCU).
 * @return Temperature of the MCU (in degrees Celsius).
 */
float Analog_getTempMCU();

/**
 * @brief Update the analog measurements.
 * @param[out] analog Pointer to a ::Analog_meas_t structure where the measurements will be stored.
 */
void Analog_update(Analog_meas_t *);
