#pragma once

/**
 * @brief Data structure representing analog measurements.
 */
typedef struct{
	uint8_t IGN1_det;	/*!<  Detection status of IGN1. */
	uint8_t IGN2_det;	/*!<  Detection status of IGN2. */
	uint8_t IGN3_det;	/*!<  Detection status of IGN3. */
	uint8_t IGN4_det;	/*!<  Detection status of IGN4. */
	uint32_t vbat_mV;	/*!<  Battery voltage (in millivolts). */
	float temp;			/*!<  Temperature (in degrees Celsius). */
} Analog_meas_t;

/**
 * @brief Initialize the analog measurement module.
 * @param ign_det_thr_val Threshold value for IGN detection.
 * @param filter Filter constant for the temperature measurement.
 * @return ESP_OK if initialization was successful, ESP_FAIL otherwise.
 */
esp_err_t Analog_init(uint32_t ign_det_thr_val, float filter);

/**
 * @brief Get the value of IGN1 detection circuit voltage.
 * @return IGN1 detection voltage.
 */
uint32_t Analog_getIGN1();

/**
 * @brief Get the value of IGN2 detection circuit voltage.
 * @return IGN2 detection voltage.
 */
uint32_t Analog_getIGN2();

/**
 * @brief Get the value of IGN3 detection circuit voltage.
 * @return IGN3 detection voltage.
 */
uint32_t Analog_getIGN3();

/**
 * @brief Get the value of IGN4 detection circuit voltage.
 * @return IGN4 detection voltage.
 */
uint32_t Analog_getIGN4();

/**
 * @brief Get the value of the battery voltage (VBAT).
 * @return Value of VBAT (in millivolts).
 */
uint32_t Analog_getVBAT();

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
