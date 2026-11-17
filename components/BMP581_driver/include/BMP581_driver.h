#pragma once

#include "esp_err.h"

/**
 * @brief BMP581 measurement. Pressure is pascals, temperature is degrees Celsius.
 */
typedef struct {
	float temp;
	float press;
} BMP581_meas_t;

/**
 * @brief Initialize the BMP581, or return ESP_OK immediately when the board has no chip-select.
 */
esp_err_t BMP581_init(void);

/**
 * @brief Read the latest compensated pressure and temperature.
 */
esp_err_t BMP581_getReloadSmart(void);

/**
 * @brief Copy the latest measurement. sensor must be 0.
 */
esp_err_t BMP581_getMeas(uint8_t sensor, BMP581_meas_t *meas);
