#pragma once

/**
 * @brief Data structure representing analog measurements.
 */
typedef struct{
	uint32_t vbat_mV;
	int8_t IGN_det[IGN_NUM];
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

/**
 * @brief Read the ignition detection state for a single channel.
 * @brief Convenience wrapper around meas->IGN_det[].
 *
 * @param meas    Measurement struct previously filled by Analog_update().
 * @param ign_no  Channel index (0-based, must be < IGN_NUM).
 * @return  1 = ignition detected, 0 = not detected, -1 = invalid/error.
 */
int8_t Analog_getIGNstate(Analog_meas_t * meas, uint8_t ign_no);
