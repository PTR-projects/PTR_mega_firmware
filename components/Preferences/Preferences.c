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


esp_err_t Preferences_init(){
	esp_err_t ret = ESP_FAIL;
	char buf[200];

	Preferences_default.mainAlt = 200;
	Preferences_default.drougeAlt = 0;

	/*ret = esp_vfs_spiffs_register(&conf_preferences);

	if(ret != ESP_OK){
		ESP_LOGE(TAG, "Failed to mount or format WWW filesystem: %s", esp_err_to_name(ret));
	}
	 */
	struct stat st;
	int8_t FileStatus = stat(preferences_path, &st);

	if(FileStatus == -1){
		ESP_LOGI(TAG, "Config file not present, created file successfully");

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
	Preferences_data_d.mainAlt = cJSON_GetObjectItem(json, "mainAlt")->valueint;
	Preferences_data_d.drougeAlt =  cJSON_GetObjectItem(json, "drougeAlt")->valueint;

	cJSON_Delete(json);

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


	Preferences_data_d = config;

	cJSON_AddNumberToObject(json, "mainAlt", Preferences_data_d.mainAlt);
	cJSON_AddNumberToObject(json, "drougeAlt", Preferences_data_d.drougeAlt);

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

	fwrite(string, 1, strlen(string), f);
	fclose(f);
	ESP_LOGI(TAG, "Updated config successfully");


	return ESP_OK;
}

































