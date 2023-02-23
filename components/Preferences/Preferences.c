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

#include "Preferences.h"

static const char *TAG = "Preferences";

esp_vfs_spiffs_conf_t conf = {
     .base_path = "/www",
     .partition_label = "www",
     .max_files = 5,
     .format_if_mount_failed = true
};

const char* base_path = "/www/preferences.json";

Preferences_driver_data_t Preferences_driver_data_d;

esp_err_t Preferences_driver_init(){
	esp_err_t ret = ESP_FAIL;
	char *buf = NULL;

	ret = esp_vfs_spiffs_register(&conf);

	if(ret != ESP_OK){
		ESP_LOGE(TAG, "Failed to mount or format WWW filesystem: %s", esp_err_to_name(ret));
	}

	struct stat st;
	int8_t FileStatus = stat(base_path, &st);

	if(FileStatus == -1){
		ESP_LOGI(TAG, "Config file not present, created file successfully");

		FILE* f = fopen(base_path, "w");
		fclose(f);

		ret = ESP_OK;
	}
	else{
			ESP_LOGE(TAG, "Config file present");
			ret = ESP_OK;
	}


	 FILE* f = fopen(base_path, "r");

	if(f == NULL){
		ESP_LOGE(TAG, "Failed to open file for reading");
		return ESP_ERR_NOT_FOUND;
	}

	fseek(f, 0L, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0L, SEEK_SET);
	fread(buf, size , 1, f);
	fclose(f);

	cJSON *json = cJSON_Parse(buf);

	Preferences_driver_data_d.mainAlt =  cJSON_GetObjectItem(json, "mainAlt")->valueint;
	Preferences_driver_data_d.drougeAlt =  cJSON_GetObjectItem(json, "drouge")->valueint;

	cJSON_Delete(json);

	return ret;
}

Preferences_driver_data_t Preferences_driver_get(){
	return Preferences_driver_data_d;
}


/*!
 * @brief Update configuration, firstly update in RAM then copy to file to prevent reading errors
 * @param config
 * Struct with new config data
 * @return `ESP_OK` if
 * @return `ESP_ERR_NOT_FOUND`
 */
esp_err_t Preferences_driver_update(Preferences_driver_data_t config){
	char *string = NULL;
	cJSON *json = cJSON_CreateObject();


	Preferences_driver_data_d.mainAlt = config.mainAlt;

	cJSON_AddNumberToObject(json, "mainAlt", Preferences_driver_data_d.mainAlt);

	return ESP_OK;
}

































