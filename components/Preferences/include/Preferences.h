#pragma once
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_event.h"

typedef struct{
	char *wifi_pass;

	int main_alt;
	int drouge_alt;

	float rail_height;
	float max_tilt;
	
	float staging_delay;
	float staging_max_tilt;

	bool auto_arming;
	int auto_arming_time_s;

	float lora_freq;
	bool lora_network_mode;


	uint32_t key;

}Preferences_data_t;

esp_err_t Preferences_init(Preferences_data_t * data);
esp_err_t Preferences_update(Preferences_data_t config);
Preferences_data_t Preferences_get();
esp_err_t Preferences_restore_dafaults();
esp_err_t Prefences_update_web(char *buf);