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

	if(json == NULL){
		ESP_LOGE(TAG, "Cannot read json!");
		return ESP_FAIL;
	}

	//Parse CMD
	if(NULL == cJSON_GetObjectItem(json, "cmd")){
		ESP_LOGE(TAG, "Cannot read cmd!");
		return ESP_FAIL;
	}


	char *cmd = cJSON_GetObjectItem(json, "cmd")->valuestring;
	ESP_LOGV(TAG, "Command: %s", cmd);

	//Parse Key
	if(NULL == cJSON_GetObjectItem(json, "key")){
		ESP_LOGE(TAG, "Cannot read key!");
		return ESP_FAIL;
	}

	uint32_t key =  cJSON_GetObjectItem(json, "key")->valueint;
	ESP_LOGV(TAG, "Key: %lu", key);

	if(key != Web_driver_cmd_d.key){
		ESP_LOGE(TAG, "Wrong key, given value was: %lu", key);
		return ESP_FAIL;
	}

	//Parse Arg1
	
	if(strcmp(cmd,"ign_set") == 0){
		if(NULL == cJSON_GetObjectItem(json, "arg1")){
			ESP_LOGE(TAG, "Cannot read arg1!");
			return ESP_FAIL;
		}

		int32_t arg1 =  cJSON_GetObjectItem(json, "arg1")->valueint;
		switch(arg1){
			case 1:
				return IGN_handle(arg1);
				return ESP_OK;
			break;

			case 2:
				return IGN_handle(arg1);
				return ESP_OK;
			break;

			case 3:
				return IGN_handle(arg1);
				return ESP_OK;
			break;

			case 4:
				return IGN_handle(arg1);
				return ESP_OK;
			break;

			default:
				ESP_LOGE(TAG, "Cannot fire igniter, bad arguments!");
				return ESP_FAIL;
			break;
		}
	}


	//Replikacja funkcji programu Areconfig
	if(strcmp(cmd,"main_alt_set") == 0){
		if(NULL == cJSON_GetObjectItem(json, "arg1")){
			ESP_LOGE(TAG, "Cannot read arg1!");
			return ESP_FAIL;
		}

		int32_t arg1 = cJSON_GetObjectItem(json, "arg1")->valueint;
		Preferences_data_t temp = Preferences_get();
		temp.main_alt = arg1;
		Preferences_update(temp);
		return ESP_OK;
		//switch main parachute altidude to given value
	}

	if(strcmp(cmd,"drouge_alt_set") == 0){
		if(NULL == cJSON_GetObjectItem(json, "arg1")){
			ESP_LOGE(TAG, "Cannot read arg1!");
			return ESP_FAIL;
		}

		int32_t arg1 = cJSON_GetObjectItem(json, "arg1")->valueint;
		Preferences_data_t temp = Preferences_get();
		temp.drouge_alt = arg1;
		Preferences_update(temp);
		return ESP_OK;
		//switch drouge parachute altidude to given value
	}

	if(strcmp(cmd,"rail_height_set") == 0){
		if(NULL == cJSON_GetObjectItem(json, "arg1")){
			ESP_LOGE(TAG, "Cannot read arg1!");
			return ESP_FAIL;
		}

		int32_t arg1 = cJSON_GetObjectItem(json, "arg1")->valueint;
		Preferences_data_t temp = Preferences_get();
		temp.rail_height = arg1;
		Preferences_update(temp);
		return ESP_OK;
		//Set launch rail height to given value
	}

	if(strcmp(cmd,"max_tilt_set") == 0){
		if(NULL == cJSON_GetObjectItem(json, "arg1")){
			ESP_LOGE(TAG, "Cannot read arg1!");
			return ESP_FAIL;
		}

		int32_t arg1 = cJSON_GetObjectItem(json, "arg1")->valueint;
		Preferences_data_t temp = Preferences_get();
		temp.max_tilt = arg1;
		Preferences_update(temp);
		return ESP_OK;
		//Set angle at which failsafe triggers
	}

	if(strcmp(cmd,"config_default") == 0){
		Preferences_restore_dafaults();
		return ESP_OK;
		//Reset config to default
	}

	cJSON_Delete(json);

	ESP_LOGW(TAG, "Command not supported");
	return ESP_OK;
}


esp_err_t IGN_handle(uint8_t ign_no){
	int8_t status = IGN_getState(ign_no);
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

	return ESP_OK;
}

