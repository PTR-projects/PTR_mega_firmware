#include "Web_driver_json.h"

#include <stdio.h>
#include <string.h>
#include "cJSON.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_event.h"


static const char *TAG = "Web_driver_json";

/*!
 * @brief Create json string and fill it with status values.
 * @return string* with json
 */
char* Web_driver_json_statusCreate(Web_driver_status_t status){

	char *string = NULL;

	cJSON *json = cJSON_CreateObject();

	cJSON *configuration = cJSON_CreateObject();
	cJSON_AddNumberToObject(configuration, "serial_number", status.serial_number);
	cJSON_AddNumberToObject(configuration, "software_version", status.software_version);
	cJSON_AddItemToObject(json, "configuration", configuration);

	cJSON *system = cJSON_CreateObject();
	cJSON_AddNumberToObject(system, "timestamp_ms", status.timestamp_ms);
	cJSON_AddNumberToObject(system, "flight_state", status.flight_state);
	cJSON_AddNumberToObject(system, "battery_voltage", status.battery_voltage);
	cJSON_AddItemToObject(json, "system", system);


	cJSON *sysMgr = cJSON_CreateObject();
	cJSON_AddNumberToObject(sysMgr, "sysmgr_system_status",  status.sysmgr_system_status);
	cJSON_AddNumberToObject(sysMgr, "sysmgr_analog_status",  status.sysmgr_analog_status);
	cJSON_AddNumberToObject(sysMgr, "sysmgr_lora_status", 	 status.sysmgr_lora_status);
	cJSON_AddNumberToObject(sysMgr, "sysmgr_adcs_status", 	 status.sysmgr_adcs_status);
	cJSON_AddNumberToObject(sysMgr, "sysmgr_storage_status", status.sysmgr_storage_status);
	cJSON_AddNumberToObject(sysMgr, "sysmgr_sysmgr_status",  status.sysmgr_sysmgr_status);
	cJSON_AddNumberToObject(sysMgr, "sysmgr_utils_status", 	 status.sysmgr_utils_status);
	cJSON_AddNumberToObject(sysMgr, "sysmgr_web_status", 	 status.sysmgr_web_status);
	cJSON_AddNumberToObject(sysMgr, "sysmgr_arm_state", 	 status.sysmgr_arm_state);
	cJSON_AddItemToObject  (json,   "sysMgr", 			     sysMgr);

	cJSON *sensors = cJSON_CreateObject();
	cJSON_AddNumberToObject(sensors, "pressure", status.pressure);
	cJSON_AddNumberToObject(sensors, "rocket_tilt", status.rocket_tilt);
	cJSON_AddNumberToObject(sensors, "gpsfix", status.gps_fix);
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

/*!
 * @brief Create json string and fill it with live values.
 * @return string* with json
 */
char* Web_driver_json_liveCreate(Web_driver_live_t live){

	char *string = NULL;
	char temp[50];

	cJSON *json = cJSON_CreateObject();

	cJSON *Global = cJSON_CreateObject();
	cJSON_AddNumberToObject(Global, "timestamp", live.timestamp);
	cJSON_AddItemToObject(json, "Global", Global);

	cJSON *MS5607 = cJSON_CreateObject();
	cJSON_AddNumberToObject(MS5607, "pressure", live.MS5607.pressure);
	cJSON_AddNumberToObject(MS5607, "altitude", live.MS5607.altitude);
	cJSON_AddNumberToObject(MS5607, "temperature", live.MS5607.temperature);
	cJSON_AddItemToObject(json, "MS5607", MS5607);


	cJSON *LIS331 = cJSON_CreateObject();
	cJSON_AddNumberToObject(LIS331, "ax", live.LIS331.ax);
	cJSON_AddNumberToObject(LIS331, "ay", live.LIS331.ay);
	cJSON_AddNumberToObject(LIS331, "az", live.LIS331.az);
	cJSON_AddItemToObject(json, "LIS331", LIS331);


	cJSON *LSM6DS32_0 = cJSON_CreateObject();
	cJSON_AddNumberToObject(LSM6DS32_0, "ax", live.LSM6DS32_0.ax);
	cJSON_AddNumberToObject(LSM6DS32_0, "ay", live.LSM6DS32_0.ay);
	cJSON_AddNumberToObject(LSM6DS32_0, "az", live.LSM6DS32_0.az);
	cJSON_AddNumberToObject(LSM6DS32_0, "gx", live.LSM6DS32_0.gx);
	cJSON_AddNumberToObject(LSM6DS32_0, "gy", live.LSM6DS32_0.gy);
	cJSON_AddNumberToObject(LSM6DS32_0, "gz", live.LSM6DS32_0.gz);
	cJSON_AddNumberToObject(LSM6DS32_0, "temperature", live.LSM6DS32_0.temperature);
	cJSON_AddItemToObject(json, "LSM6DS32_0", LSM6DS32_0);


	cJSON *LSM6DS32_1 = cJSON_CreateObject();
	cJSON_AddNumberToObject(LSM6DS32_1, "ax", live.LSM6DS32_1.ax);
	cJSON_AddNumberToObject(LSM6DS32_1, "ay", live.LSM6DS32_1.ay);
	cJSON_AddNumberToObject(LSM6DS32_1, "az", live.LSM6DS32_1.az);
	cJSON_AddNumberToObject(LSM6DS32_1, "gx", live.LSM6DS32_1.gx);
	cJSON_AddNumberToObject(LSM6DS32_1, "gy", live.LSM6DS32_1.gy);
	cJSON_AddNumberToObject(LSM6DS32_1, "gz", live.LSM6DS32_1.gz);
	cJSON_AddNumberToObject(LSM6DS32_1, "temperature", live.LSM6DS32_1.temperature);
	cJSON_AddItemToObject(json, "LSM6DS32_1", LSM6DS32_1);


	cJSON *MMC5983MA = cJSON_CreateObject();
	cJSON_AddNumberToObject(MMC5983MA, "mx", live.MMC5983MA.mx);
	cJSON_AddNumberToObject(MMC5983MA, "my", live.MMC5983MA.my);
	cJSON_AddNumberToObject(MMC5983MA, "mz", live.MMC5983MA.mz);
	cJSON_AddItemToObject(json, "MMC5983MA", MMC5983MA);


	cJSON *gps = cJSON_CreateObject();
	cJSON_AddNumberToObject(gps, "latitude", live.gps.latitude);
	cJSON_AddNumberToObject(gps, "longitude",  live.gps.longitude);
	cJSON_AddNumberToObject(gps, "fix", live.gps.fix);
	cJSON_AddNumberToObject(gps, "satellites", live.gps.sats);
	cJSON_AddItemToObject(json, "gps", gps);



	cJSON *AHRS = cJSON_CreateObject();

	cJSON_AddNumberToObject(AHRS, "anglex", live.anglex);
	cJSON_AddNumberToObject(AHRS, "angley", live.angley);
	cJSON_AddNumberToObject(AHRS, "anglez", live.anglez);
	cJSON_AddItemToObject(json, "AHRS", AHRS);


	string = cJSON_Print(json);

	if(string == NULL){
		ESP_LOGE(TAG, "Cannot create JSON string");
	}


	cJSON_Delete(json);
	ESP_LOGI(TAG, "%s", string);
	return string;
}

