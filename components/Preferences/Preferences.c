#include "include/Preferences.h"

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

#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "cJSON.h"
#include "lwip/err.h"
#include "lwip/sys.h"


static const char *TAG = "Preferences_driver";
const char* preferences_path = "/www/preferences.txt";
 

esp_vfs_spiffs_conf_t conf_preferences = {
     .base_path = "/www",
     .partition_label = "www",
     .max_files = 5,
     .format_if_mount_failed = false
};


Preferences_data_t Preferences_data_d;
Preferences_data_t Preferences_default;

uint32_t calculate_crc32(char *buf);


esp_err_t Preferences_init(Preferences_data_t * data){
	esp_err_t ret = ESP_FAIL;
	char buf[200];

	//Deafult KPPTR configuration
	Preferences_default.main_alt = 200;
	Preferences_default.drouge_alt = 0;
	Preferences_default.max_tilt = 45;
	Preferences_default.staging_delay = 0;
	Preferences_default.rail_height = 2;
	Preferences_default.auto_arming = false;
	Preferences_default.auto_arming_time_s = -1;
	*data = Preferences_default;

	struct stat st;
	int8_t FileStatus = stat(preferences_path, &st);

	if(FileStatus == -1){
		ESP_LOGI(TAG, "Config file not present, creating file");

		FILE* f = fopen(preferences_path, "w");
		fclose(f);

		Preferences_update(Preferences_default);

		ret = ESP_OK;
	}
	else{
		ESP_LOGI(TAG, "Config file present");
		ret = ESP_OK;
	}


	FILE* f = fopen(preferences_path, "r");
	if(f == NULL){
		ESP_LOGE(TAG, "Failed to open file for reading");
		return ESP_ERR_NOT_FOUND;
	}


	fseek(f, 0L, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0L, SEEK_SET);

	ESP_LOGI(TAG, "Configuration file size: %d", size);

	fread(buf, size , 1, f);
	fclose(f);

	cJSON *json = cJSON_Parse(buf);

	ESP_LOGI(TAG, "Read: %s", buf);
	if(NULL == cJSON_GetObjectItem(json, "main_alt") || 
	NULL == cJSON_GetObjectItem(json, "drouge_alt") ||
	NULL == cJSON_GetObjectItem(json, "max_tilt") ||
	NULL == cJSON_GetObjectItem(json, "staging_delay") || 
	NULL == cJSON_GetObjectItem(json, "rail_height")){
		return ESP_FAIL;
	}
	
	
	Preferences_data_d.main_alt = cJSON_GetObjectItem(json, "main_alt")->valueint;
	Preferences_data_d.drouge_alt = cJSON_GetObjectItem(json, "drouge_alt")->valueint;
	Preferences_data_d.max_tilt = cJSON_GetObjectItem(json, "max_tilt")->valueint;
	Preferences_data_d.staging_delay = cJSON_GetObjectItem(json, "staging_delay")->valueint;
	Preferences_data_d.rail_height = cJSON_GetObjectItem(json, "rail_height")->valueint;
	cJSON_Delete(json);

	*data = Preferences_data_d;
	return ret;
}

Preferences_data_t Preferences_get(){
	return Preferences_data_d;
}


/*!
 * @brief Update configuration, firstly update in RAM then copy to file to prevent reading errors
 * @param config
 * Struct with new config data
 * @return `ESP_OK` if
 * @return `ESP_ERR_NOT_FOUND`
 */
esp_err_t Preferences_update(Preferences_data_t config){
	char *string = NULL;
	cJSON *json = cJSON_CreateObject();


	//Check if all data is legit
	if(config.main_alt < 0){
		config.main_alt = 0;	//deploy on apogee
	}

	if(config.drouge_alt < 0 || config.drouge_alt < config.main_alt){
		config.drouge_alt = 0;	//deploy on apogee
	}

	if(config.rail_height > 20.0){
		config.rail_height = 20.0;	//prevent launch detection lockout
	}

	



	//Update current config stored in RAM
	Preferences_data_d = config;
	cJSON_AddStringToObject(json, "wifi_pass", Preferences_data_d.wifi_pass);
	cJSON_AddNumberToObject(json, "main_alt", Preferences_data_d.main_alt);
	cJSON_AddNumberToObject(json, "drouge_alt", Preferences_data_d.drouge_alt);
	cJSON_AddNumberToObject(json, "rail_height", Preferences_data_d.rail_height);
	cJSON_AddNumberToObject(json, "max_tilt", Preferences_data_d.max_tilt);
	cJSON_AddNumberToObject(json, "staging_delay", Preferences_data_d.staging_delay);
	cJSON_AddNumberToObject(json, "staging_max_tilt", Preferences_data_d.staging_max_tilt);
	cJSON_AddNumberToObject(json, "auto_arming_time_s", Preferences_data_d.auto_arming_time_s);
	cJSON_AddNumberToObject(json, "auto_arming", Preferences_data_d.auto_arming);
	

	string = cJSON_Print(json);
	if(string == NULL){
		ESP_LOGE(TAG, "Cannot create JSON string");
	}
	cJSON_Delete(json); 
	ESP_LOGI(TAG, "Config file: %s", string);

	FILE* f = fopen(preferences_path, "w");
	if(f == NULL){
		ESP_LOGE(TAG, "Failed to open file for writing");
		return ESP_ERR_NOT_FOUND;
	}

	//Update config stored on FLASH
	fwrite(string, 1, strlen(string), f);
	fclose(f);
	ESP_LOGI(TAG, "Updated config successfully");


	return ESP_OK;
}


esp_err_t Preferences_restore_dafaults(){
	return Preferences_update(Preferences_default);
}


esp_err_t Prefences_update_web(char *buf){
	Preferences_data_t temp;

	cJSON *json = cJSON_Parse(buf); 
	ESP_LOGI(TAG, "%s", buf);

	if(json == NULL){
		ESP_LOGE(TAG, "Cannot read json!");
		return ESP_FAIL;
	}


	if(NULL == cJSON_GetObjectItem(json, "cmd")){
		ESP_LOGE(TAG, "Cannot read cmd!");
		return ESP_FAIL;
	}
	uint32_t crc32_received = cJSON_GetObjectItem(json, "crc32")->valueint;
	cJSON_AddNumberToObject(json, "crc32", 0);

	char *string = NULL;
	string = cJSON_Print(json);
	if(string == NULL){
		ESP_LOGE(TAG, "Cannot create JSON string");
	}
	uint32_t crc32_local = calculate_crc32(string);

	if(crc32_local != crc32_received){
		ESP_LOGE(TAG, "CRC is not matching!");
		return ESP_FAIL;
	}

	if(NULL == cJSON_GetObjectItem(json, "wifi_pass")){
		ESP_LOGE(TAG, "Cannot read json!");
		return ESP_FAIL;
	}
	temp.wifi_pass = cJSON_GetObjectItem(json, "cmd")->valuestring;

	if(NULL == cJSON_GetObjectItem(json, "main_alt")){
		ESP_LOGE(TAG, "Cannot read json!");
		return ESP_FAIL;
	}
	temp.main_alt = cJSON_GetObjectItem(json, "main_alt")->valueint;

	if(NULL == cJSON_GetObjectItem(json, "drouge_alt")){
		ESP_LOGE(TAG, "Cannot read json!");
		return ESP_FAIL;
	}
	temp.drouge_alt = cJSON_GetObjectItem(json, "drouge_alt")->valueint;

	if(NULL == cJSON_GetObjectItem(json, "rail_height")){
		ESP_LOGE(TAG, "Cannot read json!");
		return ESP_FAIL;
	}
	temp.rail_height = cJSON_GetObjectItem(json, "rail_height")->valueint;

	if(NULL == cJSON_GetObjectItem(json, "max_tilt")){
		ESP_LOGE(TAG, "Cannot read json!");
		return ESP_FAIL;
	}
	temp.max_tilt = cJSON_GetObjectItem(json, "max_tilt")->valueint;

	if(NULL == cJSON_GetObjectItem(json, "staging_delay")){
		ESP_LOGE(TAG, "Cannot read json!");
		return ESP_FAIL;
	}
	temp.staging_delay = cJSON_GetObjectItem(json, "staging_delay")->valueint;

	if(NULL == cJSON_GetObjectItem(json, "staging_max_tilt")){
		ESP_LOGE(TAG, "Cannot read json!");
		return ESP_FAIL;
	}
	temp.staging_max_tilt = cJSON_GetObjectItem(json, "staging_max_tilt")->valueint;

	if(NULL == cJSON_GetObjectItem(json, "auto_arming_time_s")){
		ESP_LOGE(TAG, "Cannot read json!");
		return ESP_FAIL;
	}
	temp.auto_arming_time_s = cJSON_GetObjectItem(json, "auto_arming_time_s")->valueint;

	if(NULL == cJSON_GetObjectItem(json, "auto_arming")){
		ESP_LOGE(TAG, "Cannot read json!");
		return ESP_FAIL;
	}
	temp.auto_arming = cJSON_GetObjectItem(json, "auto_arming")->valueint;

	if(NULL == cJSON_GetObjectItem(json, "key")){
		ESP_LOGE(TAG, "Cannot read json!");
		return ESP_FAIL;
	}
	temp.key = cJSON_GetObjectItem(json, "key")->valueint;


	Preferences_update(temp);
	
	return ESP_OK;
}


uint32_t calculate_crc32(char *buf){

	int i, j;
	unsigned int byte, crc, mask;

	i = 0;
	crc = 0xFFFFFFFF;
	while (buf[i] != 0) {
		byte = buf[i];            // Get next byte.
		crc = crc ^ byte;
		for (j = 7; j >= 0; j--) {    // Do eight times.
			mask = -(crc & 1);
			crc = (crc >> 1) ^ (0xEDB88320 & mask);
		}
		i = i + 1;
	}
	return ~crc;


}
























