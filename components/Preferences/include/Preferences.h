#pragma once
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_event.h"

typedef struct{
	int main_alt;
	int drouge_alt;

	float rail_height;

	float max_tilt;

	float staging_delay;
	float staging_max_tilt;

	uint32_t key;
}Preferences_data_t;


/**
 * @brief Initialize Preferences component with default values and check for existing config file
 *
 * @return esp_err_t
 *  - ESP_OK: Success
 *  - ESP_ERR_NO_MEM: Cannot allocate memory 
 *	- ESP_FAIL: Other errors
 */
esp_err_t Preferences_init();

/*!
 * @brief Update configuration, firstly update in RAM then copy to file to prevent reading errors
 * @param config
 * Struct with new config data
 * @return esp_err_t
 *	- ESP_OK: Success 
 *	- ESP_ERR_NOT_FOUND: Cannot find file for writing
 *	- ESP_FAIL: Other errors
 */
esp_err_t Preferences_update(Preferences_data_t config);

/**
 * @brief Get struct with all the configuration data
 *
 * @return Preferences_data_t 
 *	- Configuration struct
 */
Preferences_data_t Preferences_get();

/**
 * @brief Restore deafault configuration parameters
 *
 * @return esp_err_t
 *  - ESP_OK: Success
 *  - ESP_ERR_NO_MEM: Cannot allocate memory 
 *	- ESP_FAIL: Other errors
 */
esp_err_t Preferences_restore_dafaults(); 