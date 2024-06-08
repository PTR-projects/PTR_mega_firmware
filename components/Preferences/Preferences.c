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
#include "esp_system.h"
#include <esp32/rom/crc.h>

#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "cJSON.h"
#include "lwip/err.h"
#include "lwip/sys.h"

static bool json_check_cfg(cJSON * json);

static const char *TAG = "Preferences_driver";
const char* preferences_path = "/www/preferences.txt";

static bool pref_init_done = false;
static Preferences_data_t Preferences_data_d;
static Preferences_data_t Preferences_default;
static volatile char wifi_pass[64] = { CONFIG_ESP_WIFI_PASSWORD };	// max password length for WPA2 is 63 characters - we need 1 more for '/0'

esp_err_t Preferences_init(){
	ESP_LOGI(TAG, "Preferences init");

	char buf[512] = {0};

	//Deafult KPPTR configuration
	Preferences_default.main_alt_m 			= 200;
	Preferences_default.drouge_alt_m 		= 0;
	Preferences_default.max_tilt_deg 		= 45;
	Preferences_default.staging_delay_ms 	= 0;
	Preferences_default.rail_height_mm 		= 2000;
	Preferences_default.auto_arming 		= true;
	Preferences_default.auto_arming_time_s 	= 60;
	Preferences_default.lora_freq_khz 		= 433125;
	Preferences_default.wifi_pass = (char*) wifi_pass;
	Preferences_default.lora_key 			= 0;

	// Check if configuration file exists
	struct stat st;
	int8_t FileStatus = stat(preferences_path, &st);

	if(FileStatus == -1){
		ESP_LOGW(TAG, "Config file not present, creating file");
		Preferences_update(Preferences_default, false);
	}
	else{
		ESP_LOGI(TAG, "Config file present");
	}

	// Load configuration file and parse its content
	FILE* f = fopen(preferences_path, "r");
	if(f == NULL){
		ESP_LOGE(TAG, "Failed to open file for reading");
		Preferences_data_d = Preferences_default;
		pref_init_done = true;
		return ESP_ERR_NOT_FOUND;
	}

	fseek(f, 0L, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0L, SEEK_SET);
	ESP_LOGI(TAG, "Configuration file size: %d", size);

	fread(buf, size , 1, f);	// Read file (config + 2 bytes of CRC16)
	fclose(f);

	// Check file checksum using ESP32 ROM CRC library
	uint16_t romCRC = crc16_le(0, 2+(const uint8_t*)buf, strlen(buf+2));
	ESP_LOGI(TAG, "Config file: %s", buf+2);

	if(romCRC != *((uint16_t*)buf)){
		// File corrupted
		ESP_LOGE(TAG, "CRC ERROR!");
		Preferences_update(Preferences_default, false);
		Preferences_data_d = Preferences_default;
		pref_init_done = true;
		return ESP_ERR_INVALID_CRC;
	}

	ESP_LOGI(TAG, "CRC OK!");
	cJSON *json = cJSON_Parse(buf+2);

	if(!json_check_cfg(json)) {
		ESP_LOGE(TAG, "File format not up-to-date! Changing...");
		Preferences_update(Preferences_default, false);
		Preferences_data_d = Preferences_default;
		pref_init_done = true;
		return ESP_FAIL;
	}

	ESP_LOGI(TAG, "JSON OK");
	Preferences_data_d.main_alt_m 			= cJSON_GetObjectItem(json, "main_alt")->valueint;
	Preferences_data_d.drouge_alt_m 		= cJSON_GetObjectItem(json, "drouge_alt")->valueint;
	Preferences_data_d.max_tilt_deg 		= cJSON_GetObjectItem(json, "max_tilt")->valueint;
	Preferences_data_d.staging_delay_ms 	= cJSON_GetObjectItem(json, "staging_delay")->valueint;
	Preferences_data_d.staging_max_tilt 	= cJSON_GetObjectItem(json, "staging_max_tilt")->valueint;
	Preferences_data_d.rail_height_mm 		= cJSON_GetObjectItem(json, "rail_height")->valueint;
	Preferences_data_d.auto_arming 			= cJSON_GetObjectItem(json, "auto_arming")->valueint;
	Preferences_data_d.auto_arming_time_s 	= cJSON_GetObjectItem(json, "auto_arming_time_s")->valueint;
	Preferences_data_d.lora_freq_khz 		= cJSON_GetObjectItem(json, "lora_freq")->valueint;
	Preferences_data_d.lora_mode 			= cJSON_GetObjectItem(json, "lora_mode")->valueint;
	Preferences_data_d.lora_key 			= cJSON_GetObjectItem(json, "lora_key")->valueint;
	Preferences_data_d.wifi_pass 			= (char*)wifi_pass;
	memcpy(Preferences_data_d.wifi_pass, 	  cJSON_GetObjectItem(json, "wifi_pass")->valuestring,
											  strlen(cJSON_GetObjectItem(json, "wifi_pass")->valuestring));
	cJSON_Delete(json);
	pref_init_done = true;
	
	return ESP_OK;
}

esp_err_t Preferences_get(Preferences_data_t * ptr){
	if(pref_init_done){
		*ptr = Preferences_data_d;
		return ESP_OK;
	}

	return ESP_ERR_NOT_FINISHED;
}


/*!
 * @brief Update configuration, firstly update in RAM then copy to file to prevent reading errors
 * @param config
 * Struct with new config data
 * @return `ESP_OK` if
 * @return `ESP_ERR_NOT_FOUND`
 */
esp_err_t Preferences_update(Preferences_data_t config, bool req_restart){
	ESP_LOGI(TAG, "Config update");
	char conf_file[1024] = {0};
	cJSON *json = cJSON_CreateObject();
	
	//Update current config stored in RAM
	Preferences_data_d = config;
	cJSON_AddStringToObject(json, "wifi_pass", 			Preferences_data_d.wifi_pass);
	cJSON_AddNumberToObject(json, "main_alt", 			Preferences_data_d.main_alt_m);
	cJSON_AddNumberToObject(json, "drouge_alt",			Preferences_data_d.drouge_alt_m);
	cJSON_AddNumberToObject(json, "rail_height", 		Preferences_data_d.rail_height_mm);
	cJSON_AddNumberToObject(json, "max_tilt", 			Preferences_data_d.max_tilt_deg);
	cJSON_AddNumberToObject(json, "staging_delay", 		Preferences_data_d.staging_delay_ms);
	cJSON_AddNumberToObject(json, "staging_max_tilt", 	Preferences_data_d.staging_max_tilt);
	cJSON_AddNumberToObject(json, "auto_arming_time_s", Preferences_data_d.auto_arming_time_s);
	cJSON_AddNumberToObject(json, "auto_arming", 		Preferences_data_d.auto_arming);
	cJSON_AddNumberToObject(json, "lora_freq", 			Preferences_data_d.lora_freq_khz);
	cJSON_AddNumberToObject(json, "lora_mode", 			Preferences_data_d.lora_mode);
	cJSON_AddNumberToObject(json, "lora_key", 			Preferences_data_d.lora_key);
	cJSON_AddNumberToObject(json, "key",				Preferences_data_d.lora_key);
	
	if(cJSON_PrintPreallocated(json, conf_file+2, sizeof(conf_file), 0) == 0){
		ESP_LOGE(TAG, "Cannot create JSON string");
		return ESP_FAIL;
	}
	cJSON_Delete(json); 

	// Add CRC16 at the end of the string
	uint32_t romCRC = crc16_le(0, 2+(const uint8_t*)conf_file, strlen(conf_file+2));
	*((uint16_t*)conf_file) = romCRC;

	ESP_LOGI(TAG, "New Config file: %s", conf_file+2);
	
	//Update config stored on FLASH chip
	remove(preferences_path);
	FILE* f = fopen(preferences_path, "w");
	if(f == NULL){
		ESP_LOGE(TAG, "Failed to open file for writing");
		return ESP_ERR_NOT_FOUND;
	}

	fwrite(conf_file, 1, 2+strlen(conf_file+2), f);
	fwrite("\0", 1, 1, f);
	fclose(f);
	ESP_LOGI(TAG, "Updated config successfully");
	
	if(req_restart){
		esp_restart();
	}

	return ESP_FAIL; //If it returns it means that esp failed to restart
}

esp_err_t Preferences_restore_dafaults(){
	return Preferences_update(Preferences_default, false);
}

esp_err_t Prefences_update_web(char *buf){
	if(buf == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	Preferences_data_t temp = Preferences_data_d;
	uint16_t crc_from_json = atoi(buf);

	buf = strchr(buf, '{');
	if(buf == NULL) {
		return ESP_FAIL;
	}

	cJSON *json = cJSON_Parse(buf);
	ESP_LOGI(TAG, "New from web:\n%s", buf);

	if(!json_check_cfg(json)) {
		ESP_LOGE(TAG, "Cannot read json!");
		return ESP_FAIL;
	}
	
	uint16_t crc16_received = crc_from_json;
	uint16_t romCRC = ~crc16_be((uint16_t)~0xffff, (const uint8_t*)buf, strlen(buf));

	if(crc16_received != romCRC) {
		ESP_LOGE(TAG, "CRC error");
		return ESP_FAIL;
	}

	ESP_LOGI(TAG, "JSON CRC OK: 0x%x", crc16_received);
	temp.wifi_pass 			= cJSON_GetObjectItem(json, "wifi_pass")->valuestring;
	temp.main_alt_m 		= cJSON_GetObjectItem(json, "main_alt")->valueint;
	temp.drouge_alt_m 		= cJSON_GetObjectItem(json, "drouge_alt")->valueint;
	temp.rail_height_mm 	= cJSON_GetObjectItem(json, "rail_height")->valueint;
	temp.max_tilt_deg 		= cJSON_GetObjectItem(json, "max_tilt")->valueint;
	temp.staging_delay_ms 	= cJSON_GetObjectItem(json, "staging_delay")->valueint;
	temp.staging_max_tilt 	= cJSON_GetObjectItem(json, "staging_max_tilt")->valueint;
	temp.auto_arming_time_s = cJSON_GetObjectItem(json, "auto_arming_time_s")->valueint;
	temp.auto_arming 		= cJSON_GetObjectItem(json, "auto_arming")->valueint;
	temp.key 				= cJSON_GetObjectItem(json, "key")->valueint;
	temp.lora_freq_khz 		= cJSON_GetObjectItem(json, "lora_freq")->valueint;
	temp.lora_mode 			= cJSON_GetObjectItem(json, "lora_mode")->valueint;
	temp.lora_key 			= cJSON_GetObjectItem(json, "lora_key")->valueint;

	return Preferences_update(temp, true);
}

/**
 * @brief Checks if provided configuration JSON is valid. Pointer must not be NULL
 * and all necessary data must be present.
 *
 * @param json Pointer to configuration JSON to be check
 * @return bool True = JSON valid, False = invalid
 */
static bool json_check_cfg(cJSON * json){
	if(json == NULL) {
		return false;
	}

	bool json_check = false;
	json_check |= (NULL == cJSON_GetObjectItem(json, "main_alt"));
	json_check |= (NULL == cJSON_GetObjectItem(json, "max_tilt"));
	json_check |= (NULL == cJSON_GetObjectItem(json, "staging_delay"));
	json_check |= (NULL == cJSON_GetObjectItem(json, "staging_max_tilt"));
	json_check |= (NULL == cJSON_GetObjectItem(json, "rail_height"));
	json_check |= (NULL == cJSON_GetObjectItem(json, "auto_arming"));
	json_check |= (NULL == cJSON_GetObjectItem(json, "auto_arming_time_s"));
	json_check |= (NULL == cJSON_GetObjectItem(json, "lora_freq"));
	json_check |= (NULL == cJSON_GetObjectItem(json, "lora_mode"));
	json_check |= (NULL == cJSON_GetObjectItem(json, "lora_key"));
	json_check |= (NULL == cJSON_GetObjectItem(json, "wifi_pass"));

	return !json_check;
}
