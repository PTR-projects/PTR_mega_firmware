#pragma once
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_event.h"

typedef struct{
	int main_alt;
	int drouge_alt;

	float rail_height;

	float max_tilt;

	float staging_delay;
	float staging_max_tilt;

	uint32_t key;
}Preferences_data_t;

esp_err_t Preferences_init();
esp_err_t Preferences_update(Preferences_data_t config);
Preferences_data_t Preferences_get();
esp_err_t Preferences_restore_dafaults();