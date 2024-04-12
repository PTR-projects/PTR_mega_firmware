#pragma once

typedef struct {
	uint32_t key;
} Web_driver_cmd_t;


/**
* @brief Handle command sent by external interface
* @param[in] buf buffer with command in JSON format
* @return esp_err_t
*	- ESP_OK:  Success 
*	- ESP_FAIL: Fail
*/
esp_err_t Web_cmd_handler(char *buf);

/**
* @brief Initialize CMD component
*
* @return esp_err_t
*	- ESP_OK:  Success 
*/
esp_err_t Web_cmd_init(uint32_t key);
