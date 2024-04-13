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

static const char *TAG = "Preferences_driver";
const char* preferences_path = "/www/preferences.txt";

Preferences_data_t Preferences_data_d;
Preferences_data_t Preferences_default;
static volatile char wifi_pass[64] = {CONFIG_ESP_WIFI_PASSWORD};	// max password length for WPA2 is 63 characters - we need 1 more for '/0'

esp_err_t Preferences_init(){
	ESP_LOGI(TAG, "Preferences init");

	esp_err_t ret = ESP_FAIL;
	char buf[512];

	//Deafult KPPTR configuration
	Preferences_default.main_alt = 200;
	Preferences_default.drouge_alt = 0;
	Preferences_default.max_tilt = 45;
	Preferences_default.staging_delay = 0;
	Preferences_default.rail_height = 2;
	Preferences_default.auto_arming = true;
	Preferences_default.auto_arming_time_s = 60;
	Preferences_default.lora_freq = 433125;
	Preferences_default.wifi_pass = (char*) wifi_pass;

	// Check if configuration file exists
	struct stat st;
	int8_t FileStatus = stat(preferences_path, &st);

	if(FileStatus == -1){
		ESP_LOGW(TAG, "Config file not present, creating file");
		Preferences_update(Preferences_default);
		ret = ESP_OK;
	}
	else{
		ESP_LOGI(TAG, "Config file present");
		ret = ESP_OK;
	}

	// Load configuration file and parse its content
	FILE* f = fopen(preferences_path, "r");
	if(f == NULL){
		ESP_LOGE(TAG, "Failed to open file for reading");
		Preferences_data_d = Preferences_default;
		return ESP_ERR_NOT_FOUND;
	}

	fseek(f, 0L, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0L, SEEK_SET);
	ESP_LOGI(TAG, "Configuration file size: %d", size);

	fread(buf, size , 1, f);	// Read file (config + 2 bytes of CRC16)
	fclose(f);

	// Check file checksum using ESP32 ROM CRC library
	uint16_t romCRC = crc16_le(0, 2+(const uint8_t*)buf, size-2);
	ESP_LOGI(TAG, "Config file: %s", buf+2);
	ESP_LOGI(TAG, "Config CRC:  0x%x", *((uint16_t*)buf));
	ESP_LOGI(TAG, "ROM CRC:  0x%x", romCRC);

	if(romCRC == *((uint16_t*)buf)){
		ESP_LOGI(TAG, "CRC OK!");
		cJSON *json = cJSON_Parse(buf+2);

		ESP_LOGI(TAG, "Read: %s", buf+2);
		if(	  NULL == cJSON_GetObjectItem(json, "main_alt")
		   || NULL == cJSON_GetObjectItem(json, "drouge_alt")
		   || NULL == cJSON_GetObjectItem(json, "max_tilt")
		   || NULL == cJSON_GetObjectItem(json, "staging_delay")
		   || NULL == cJSON_GetObjectItem(json, "rail_height")
		   || NULL == cJSON_GetObjectItem(json, "auto_arming")
		   || NULL == cJSON_GetObjectItem(json, "auto_arming_time_s")
		   || NULL == cJSON_GetObjectItem(json, "lora_freq")
		   || NULL == cJSON_GetObjectItem(json, "wifi_pass"))
		{
				return ESP_FAIL;
		}
		ESP_LOGI(TAG, "JSON OK");
		Preferences_data_d.main_alt 			= cJSON_GetObjectItem(json, "main_alt")->valueint;
		Preferences_data_d.drouge_alt 			= cJSON_GetObjectItem(json, "drouge_alt")->valueint;
		Preferences_data_d.max_tilt 			= cJSON_GetObjectItem(json, "max_tilt")->valueint;
		Preferences_data_d.staging_delay 		= cJSON_GetObjectItem(json, "staging_delay")->valueint;
		Preferences_data_d.rail_height 			= cJSON_GetObjectItem(json, "rail_height")->valueint;
		Preferences_data_d.auto_arming 			= cJSON_GetObjectItem(json, "auto_arming")->valueint;
		Preferences_data_d.auto_arming_time_s 	= cJSON_GetObjectItem(json, "auto_arming_time_s")->valueint;
		Preferences_data_d.lora_freq 			= cJSON_GetObjectItem(json, "lora_freq")->valueint;
		Preferences_data_d.wifi_pass 			= (char*)wifi_pass;
		memcpy(Preferences_data_d.wifi_pass, cJSON_GetObjectItem(json, "wifi_pass")->valuestring,
				strlen(cJSON_GetObjectItem(json, "wifi_pass")->valuestring));
		cJSON_Delete(json);
	}
	else {	// file corrupted
		ESP_LOGE(TAG, "CRC ERROR!");
		Preferences_update(Preferences_default);
		Preferences_data_d = Preferences_default;
		return ESP_ERR_INVALID_CRC;
	}
	
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
	ESP_LOGI(TAG, "Config update");
	char conf_file[1024] = {0};
	cJSON *json = cJSON_CreateObject();
	
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
	cJSON_AddNumberToObject(json, "lora_freq", Preferences_data_d.lora_freq);
	cJSON_AddNumberToObject(json, "lora_mode", 0);
	cJSON_AddNumberToObject(json, "key", 2137);
	
	if(cJSON_PrintPreallocated(json, conf_file+2, sizeof(conf_file), 0) == 0){
		ESP_LOGE(TAG, "Cannot create JSON string");
		return ESP_FAIL;
	}
	cJSON_Delete(json); 

	// Add CRC16 at the end of the string
	uint32_t romCRC = crc16_le(0, 2+(const uint8_t*)conf_file, strlen(conf_file+2));
	*((uint16_t*)conf_file) = romCRC;

	ESP_LOGI(TAG, "New Config size: %i", 2+strlen(conf_file+2));
	ESP_LOGI(TAG, "New Config file: %s", conf_file+2);
	ESP_LOGI(TAG, "New Config CRC:  0x%x", *((uint16_t*)conf_file));
	
	//Update config stored on FLASH chip
	FILE* f = fopen(preferences_path, "w");
	if(f == NULL){
		ESP_LOGE(TAG, "Failed to open file for writing");
		return ESP_ERR_NOT_FOUND;
	}

	fwrite(conf_file, 1, 2+strlen(conf_file+2), f);
	fclose(f);
	ESP_LOGI(TAG, "Updated config successfully");
	
	//esp_restart();

	return ESP_FAIL; //If it returns it means that esp failed to restart
}

esp_err_t Preferences_restore_dafaults(){
	return Preferences_update(Preferences_default);
}

esp_err_t Prefences_update_web(char *buf){
	if(buf == NULL)
		return ESP_ERR_INVALID_ARG;

	Preferences_data_t temp = Preferences_data_d;

	cJSON *json = cJSON_Parse(buf+2);
	ESP_LOGE(TAG, "%s", buf+2);

	if(json == NULL
			|| NULL == cJSON_GetObjectItem(json, "crc32")
			|| NULL == cJSON_GetObjectItem(json, "wifi_pass")
			|| NULL == cJSON_GetObjectItem(json, "main_alt")
			|| NULL == cJSON_GetObjectItem(json, "drouge_alt")
			|| NULL == cJSON_GetObjectItem(json, "rail_height")
			|| NULL == cJSON_GetObjectItem(json, "max_tilt")
			|| NULL == cJSON_GetObjectItem(json, "staging_delay")
			|| NULL == cJSON_GetObjectItem(json, "staging_max_tilt")
			|| NULL == cJSON_GetObjectItem(json, "auto_arming_time_s")
			|| NULL == cJSON_GetObjectItem(json, "auto_arming")
			|| NULL == cJSON_GetObjectItem(json, "key")
			|| NULL == cJSON_GetObjectItem(json, "lora_freq"))
	{
		ESP_LOGE(TAG, "Cannot read json!");
		return ESP_FAIL;
	}
	
	uint16_t crc16_received = *((uint16_t*)buf);
	uint16_t romCRC = crc16_le(0, 2+(const uint8_t*)buf, strlen(buf+2));
	
	if(crc16_received == romCRC){
		temp.wifi_pass 			= cJSON_GetObjectItem(json, "wifi_pass")->valuestring;
		temp.main_alt 			= cJSON_GetObjectItem(json, "main_alt")->valueint;
		temp.drouge_alt 		= cJSON_GetObjectItem(json, "drouge_alt")->valueint;
		temp.rail_height 		= cJSON_GetObjectItem(json, "rail_height")->valueint;
		temp.max_tilt 			= cJSON_GetObjectItem(json, "max_tilt")->valueint;
		temp.staging_delay 		= cJSON_GetObjectItem(json, "staging_delay")->valueint;
		temp.staging_max_tilt 	= cJSON_GetObjectItem(json, "staging_max_tilt")->valueint;
		temp.auto_arming_time_s = cJSON_GetObjectItem(json, "auto_arming_time_s")->valueint;
		temp.auto_arming 		= cJSON_GetObjectItem(json, "auto_arming")->valueint;
		temp.key 				= cJSON_GetObjectItem(json, "key")->valueint;
		temp.lora_freq 			= cJSON_GetObjectItem(json, "lora_freq")->valueint;

		return Preferences_update(temp);
	}

	return ESP_FAIL;
}

char* Preferences_send_config_web(){
	char *string = NULL;
	cJSON *json = cJSON_CreateObject();

	cJSON_AddStringToObject(json, "wifi_pass", 			Preferences_data_d.wifi_pass);
	cJSON_AddNumberToObject(json, "main_alt", 			Preferences_data_d.main_alt);
	cJSON_AddNumberToObject(json, "drouge_alt", 		Preferences_data_d.drouge_alt);
	cJSON_AddNumberToObject(json, "rail_height", 		Preferences_data_d.rail_height);
	cJSON_AddNumberToObject(json, "max_tilt", 			Preferences_data_d.max_tilt);
	cJSON_AddNumberToObject(json, "staging_delay", 		Preferences_data_d.staging_delay);
	cJSON_AddNumberToObject(json, "staging_max_tilt", 	Preferences_data_d.staging_max_tilt);
	cJSON_AddNumberToObject(json, "auto_arming_time_s", Preferences_data_d.auto_arming_time_s);
	cJSON_AddNumberToObject(json, "auto_arming", 		Preferences_data_d.auto_arming);
	cJSON_AddNumberToObject(json, "lora_freq", 			Preferences_data_d.lora_freq);
	cJSON_AddNumberToObject(json, "lora_mode", 			0);
	cJSON_AddNumberToObject(json, "key", 				2137);
	
	string = cJSON_Print(json);
	if(string == NULL){
		ESP_LOGE(TAG, "Cannot create JSON string");
		return "";
	}
	cJSON_Delete(json); 
	ESP_LOGI(TAG, "Config file: %s", string);

	return string;
}

