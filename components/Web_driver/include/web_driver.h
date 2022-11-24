#pragma once
#include "esp_err.h"


typedef struct{
	char password[20];
	char ssid[20];
} Web_data_t;

typedef struct{
	bool IGN_status[5];
} Rocket_status_t;

esp_err_t Web_init(void);
