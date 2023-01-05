#include "Web_driver_json.h"

#include <stdio.h>
#include "cJSON.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_event.h"


static const char *TAG = "Web_driver_json";

Web_driver_config_t Web_driver_config_d;


esp_err_t Web_driver_json_init(Web_driver_config_t config){

	 Web_driver_config_d.serialNumber = config.serialNumber;
	 Web_driver_config_d.softwareVersion = config.softwareVersion;

	return ESP_OK;
}



char* Web_driver_json_create(Web_driver_status_t status){

	char *string = NULL;
	char temp[50];

	cJSON *json = cJSON_CreateObject();

	cJSON *configuration = cJSON_CreateObject();

	cJSON_AddStringToObject(configuration, "serialNumber", "1234");
	cJSON_AddStringToObject(configuration, "softwareVersion", "1.0");

	cJSON_AddItemToObject(json, "configuration", configuration);


	cJSON *logic = cJSON_CreateObject();

	sprintf(temp, "%d", status.state);
	cJSON_AddStringToObject(logic, "state", temp);
	sprintf(temp, "%d", status.timestamp);
	cJSON_AddStringToObject(logic, "timestamp", temp);

	cJSON_AddItemToObject(json, "logic", logic);


	cJSON *sysMgr = cJSON_CreateObject();

	cJSON_AddStringToObject(sysMgr, "Storage_driver", "OK");
	cJSON_AddStringToObject(sysMgr, "Web_driver", "OK");

	cJSON_AddItemToObject(json, "sysMgr", sysMgr);


	cJSON *sensors = cJSON_CreateObject();

	sprintf(temp, "%f", status.pressure);
	cJSON_AddStringToObject(sensors, "pressure", temp);

	sprintf(temp, "%f", status.altitude);
	cJSON_AddStringToObject(sensors, "altitude", temp);

	sprintf(temp, "%f", status.angle);
	cJSON_AddStringToObject(sensors, "angle", temp);

	cJSON *gps = cJSON_CreateObject();

	cJSON *latitude = cJSON_CreateObject();
	sprintf(temp, "%f", status.latitude.value);
	cJSON_AddStringToObject(latitude, "value", temp);
	sprintf(temp, status.latitude.direction);
	cJSON_AddStringToObject(latitude, "direction", temp);
	cJSON_AddItemToObject(gps, "latitude", latitude);

	cJSON *longitude = cJSON_CreateObject();
	sprintf(temp, "%f", status.longitude.value);
	cJSON_AddStringToObject(longitude, "value", temp);
	sprintf(temp, status.longitude.direction);
	cJSON_AddStringToObject(longitude, "direction", temp);
	cJSON_AddItemToObject(gps, "longitude", longitude);

	cJSON_AddItemToObject(sensors, "gps", gps);


	cJSON_AddItemToObject(json, "sensors", sensors);



	string = cJSON_Print(json);

	if(string == NULL){
		ESP_LOGE(TAG, "Cannot create json string");
	}


	cJSON_Delete(json);
	ESP_LOGI(TAG, "%s", string);
	return string;
}
