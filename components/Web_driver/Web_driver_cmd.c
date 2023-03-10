#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_event.h"

#include "cJSON.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "IGN_driver.h"
#include "Web_driver.h"
#include "Web_driver_json.h"
#include "Web_driver_cmd.h"
#include "Preferences.h"

static const char *TAG = "Web_driver_cmd";

Web_driver_cmd_t Web_driver_cmd_d;

esp_err_t IGN_handle(uint8_t ign_no);

esp_err_t Web_cmd_init(uint32_t key){
	Web_driver_cmd_d.key = key;
	return ESP_OK;
}

/*!
 * @brief Handle commands from json string.
 * @return `ESP_OK` if initialized
 * @return `ESP_FAIL` otherwise.
 */
esp_err_t Web_cmd_handler(char *buf){
	cJSON *json = cJSON_Parse(buf);
	ESP_LOGI(TAG, "%s", buf);


	char *cmd = cJSON_GetObjectItem(json, "cmd")->valuestring;
	ESP_LOGI(TAG, "Command: %s", cmd);
	uint32_t key =  cJSON_GetObjectItem(json, "key")->valueint;
	ESP_LOGI(TAG, "Key: %d", key);

	if(key != Web_driver_cmd_d.key){
		ESP_LOGE(TAG, "Wrong key, given value was: %d", key);
		return ESP_FAIL;
	}

	if(strcmp(cmd,"ign_set") == 0){
		int32_t arg1 =  cJSON_GetObjectItem(json, "arg1")->valueint;
		switch(arg1){
			case 1:
				return IGN_handle(arg1);
			break;

			case 2:
				return IGN_handle(arg1);
			break;

			case 3:
				return IGN_handle(arg1);
			break;

			case 4:
				return IGN_handle(arg1);
			break;

			default:
				ESP_LOGE(TAG, "Cannot fire igniter, bad arguments!");
				return ESP_FAIL;
			break;

		}
	}


	//Replikacja funkcji programu Areconfig
	if(strcmp(cmd,"main_alt_set") == 0){
		int32_t arg1 = cJSON_GetObjectItem(json, "arg1")->valueint;
		Preferences_data_t temp = Preferences_get();
		temp.main_alt = arg1;
		Preferences_update(temp);
		//switch main parachute altidude to given value
	}

	if(strcmp(cmd,"drouge_alt_set") == 0){
		int32_t arg1 = cJSON_GetObjectItem(json, "arg1")->valueint;
		Preferences_data_t temp = Preferences_get();
		temp.drouge_alt = arg1;
		Preferences_update(temp);
		//switch drouge parachute altidude to given value
	}

	if(strcmp(cmd,"launch_rail_height_set") == 0){
		int32_t arg1 = cJSON_GetObjectItem(json, "arg1")->valueint;
		Preferences_data_t temp = Preferences_get();
		temp.rail_height = arg1;
		Preferences_update(temp);
		//Set launch rail height to given value
	}

	if(strcmp(cmd,"max_tilt_set") == 0){
		int32_t arg1 = cJSON_GetObjectItem(json, "arg1")->valueint;
		Preferences_data_t temp = Preferences_get();
		temp.max_tilt = arg1;
		Preferences_update(temp);
		//Set angle at which failsafe triggers
	}

	if(strcmp(cmd,"config_default") == 0){
		Preferences_restore_dafaults();
		//Reset config to default
	}

	cJSON_Delete(json);

	return ESP_OK;
}


esp_err_t IGN_handle(uint8_t ign_no){

	uint8_t status = IGN_getState(ign_no);
	if(status == 1){
		ESP_LOGE(TAG, "Igniter: %d, already up!", ign_no);
		return ESP_FAIL;
	}
	else if(status == -1){
		ESP_LOGE(TAG, "Igniter: %d, cannot check state!", ign_no);
		return ESP_FAIL;
	}

	esp_err_t ret = IGN_set(ign_no, 1);
	if(ret != ESP_OK){
		ESP_LOGI(TAG, "Igniter: %d, error during fire!", ign_no);
		return ESP_FAIL;
	}
	ESP_LOGI(TAG, "Igniter: %d, fire!", ign_no);

	uint32_t time_ms = pdTICKS_TO_MS(xTaskGetTickCount ());


	return ESP_OK;
}

