#pragma once
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_event.h"

typedef struct{
	char *wifi_pass;

	int main_alt_m;
	int drouge_alt_m;

	int rail_height_mm;
	int max_tilt_deg;
	
	int staging_delay_ms;
	int staging_max_tilt;

	bool auto_arming;
	int auto_arming_time_s;

	int lora_freq_khz;
	bool lora_mode;
	uint64_t lora_key;

	uint32_t key;

} Preferences_data_t;

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
 * @param req_restart
 * Struct with new config data
 * @return esp_err_t
 *	- ESP_OK: Success 
 *	- ESP_ERR_NOT_FOUND: Cannot find file for writing
 *	- ESP_FAIL: Other errors
 */
esp_err_t Preferences_update(Preferences_data_t config, bool req_restart);

/**
 * @brief Get struct with all the configuration data
 *
 * @param ptr Pointer to return config data
 * @return esp_err_t
 *	- ESP_OK: Success
 *	- ESP_FAIL: Config not initialized
 */
esp_err_t Preferences_get(Preferences_data_t * ptr);

/**
 * @brief Restore deafault configuration parameters
 *
 * @return esp_err_t
 *  - ESP_OK: Success
 *  - ESP_ERR_NO_MEM: Cannot allocate memory 
 *	- ESP_FAIL: Other errors
 */
esp_err_t Preferences_restore_dafaults();

esp_err_t Prefences_update_web(char *buf);
char* Preferences_send_config_web();
