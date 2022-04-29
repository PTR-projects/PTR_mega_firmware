#include <stdio.h>
#include <string.h>
#include "esp_err.h"

#include "Sensors.h"

//--------- Private var ---------------
static Sensors_t Sensors_d;

esp_err_t Sensors_init()
{
	memset(&Sensors_d, 0, sizeof(Sensors_d));

	return ESP_OK; 	//ESP_FAIL
}

esp_err_t Sensors_update(){
	//get new data from sensors

	return ESP_OK; 	//ESP_FAIL
}
