#pragma once
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_event.h"

typedef struct{
	int mainAlt;
	int drougeAlt;

	float railHeight;

	float maxAngle;

	float stagingDelay;
	float stagingMaxAngle;

	uint32_t key;
}Preferences_data_t;

esp_err_t Preferences_init();
esp_err_t Preferences_update(Preferences_data_t config);
Preferences_data_t Preferences_get();
