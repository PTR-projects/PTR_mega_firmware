#pragma once


#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_event.h"

typedef struct{
	long serialNumber;
	char softwareVersion[10];

	int state;
	int timestamp;
	int drougeAlt;
	int mainAlt;

	int sysmgr_sysmgr_status;
	int sysmgr_main_status;
	int sysmgr_storage_status;
	int sysmgr_lora_status;
	int sysmgr_analog_status;
	int sysmgr_utils_status;
	int sysmgr_web_status;

	float angle;
	float latitude;
	float longitude;
	int fix;

	float batteryVoltage;

	struct {
		bool fired;
		bool continuity;
	} igniters[4];
} Web_driver_status_t;

typedef struct{
	struct {
		float pressure;
		float altitude;
		float temperature;
	} MS5607;

	struct {
		float ax;
		float ay;
		float az;
	} LIS331;

	struct {
		float ax;
		float ay;
		float az;

		float gx;
		float gy;
		float gz;

		float temperature;
	} LSM6DS32_0, LSM6DS32_1;

	struct {
		float mx;
		float my;
		float mz;
	} MMC5983MA;

	struct {
		float latitude;
		float longitude;

		int fix;
		int sats;
	} gps;

	float anglex;
	float angley;
	float anglez;


} Web_driver_live_t;


char* Web_driver_json_statusCreate(Web_driver_status_t status);
char* Web_driver_json_liveCreate(Web_driver_live_t status);
Web_driver_status_t Web_driver_json_parse(char* json);
