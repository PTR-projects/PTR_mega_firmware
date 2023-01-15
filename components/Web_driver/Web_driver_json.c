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

	cJSON_AddNumberToObject(configuration, "serialNumber", Web_driver_config_d.serialNumber);
	cJSON_AddStringToObject(configuration, "softwareVersion", Web_driver_config_d.softwareVersion);

	cJSON_AddItemToObject(json, "configuration", configuration);


	cJSON *logic = cJSON_CreateObject();


	cJSON_AddNumberToObject(logic, "state", status.state);
	cJSON_AddNumberToObject(logic, "timestamp", status.timestamp);
	cJSON_AddNumberToObject(logic, "batteryVoltage", status.batteryVoltage);

	cJSON_AddItemToObject(json, "logic", logic);


	cJSON *sysMgr = cJSON_CreateObject();

	cJSON_AddStringToObject(sysMgr, "Storage_driver", "OK");
	cJSON_AddStringToObject(sysMgr, "Web_driver", "OK");

	cJSON_AddItemToObject(json, "sysMgr", sysMgr);


	cJSON *sensors = cJSON_CreateObject();


	cJSON_AddNumberToObject(sensors, "pressure", status.pressure);


	cJSON_AddNumberToObject(sensors, "altitude", status.altitude);


	cJSON_AddNumberToObject(sensors, "angle", status.angle);

	cJSON *gps = cJSON_CreateObject();

	cJSON *latitude = cJSON_CreateObject();
	cJSON_AddNumberToObject(latitude, "value", status.latitude.value);
	sprintf(temp, status.latitude.direction);
	cJSON_AddStringToObject(latitude, "direction", temp);
	cJSON_AddItemToObject(gps, "latitude", latitude);

	cJSON *longitude = cJSON_CreateObject();
	cJSON_AddNumberToObject(longitude, "value", status.longitude.value);
	sprintf(temp, status.longitude.direction);
	cJSON_AddStringToObject(longitude, "direction", temp);
	cJSON_AddItemToObject(gps, "longitude", longitude);

	cJSON_AddItemToObject(sensors, "gps", gps);

	cJSON_AddItemToObject(json, "sensors", sensors);


	cJSON *igniters = cJSON_CreateArray();

	for(int i=0;i<4;i++){
		cJSON *igniter = cJSON_CreateObject();

		cJSON_AddNumberToObject(igniter, "fired", status.igniters[i].fired);
		cJSON_AddNumberToObject(igniter, "continuity", status.igniters[i].continuity);

		cJSON_AddItemToArray(igniters, igniter);
	}

	cJSON_AddItemToObject(json, "igniters", igniters);




	string = cJSON_Print(json);

	if(string == NULL){
		ESP_LOGE(TAG, "Cannot create JSON string");
	}


	cJSON_Delete(json);
	ESP_LOGI(TAG, "%s", string);
	return string;
}
