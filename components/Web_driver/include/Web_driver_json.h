#pragma once

#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_event.h"

typedef struct{
	int state;
	int timestamp;
	int drougeAlt;
	int mainAlt;

	float pressure;
	float altitude;
	float angle;

	float batteryVoltage;

	struct {
		float value;
	    char *direction;
	} latitude, longitude;

	struct {
		bool fired;
		bool continuity;
	} pyro[4];

} Web_driver_status_t;

typedef struct{
	char *serialNumber;
	char *softwareVersion;
} Web_driver_config_t;

esp_err_t Web_driver_json_init(Web_driver_config_t config);

char* Web_driver_json_create(Web_driver_status_t status);
Web_driver_status_t Web_driver_json_parse(char* json);
