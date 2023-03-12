#pragma once
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_event.h"

typedef struct{
	uint64_t serial_number;
	uint64_t software_version;

	uint8_t flight_state;

	int drouge_alt;
	int main_alt;

	float battery_voltage;
	float rocket_tilt;

	struct {
		uint8_t fired;
		uint8_t continuity;
	} igniters[4];


	uint8_t sysmgr_system_status;
	uint8_t sysmgr_main_status;
	uint8_t sysmgr_analog_status;
	uint8_t	sysmgr_lora_status;
	uint8_t	sysmgr_adcs_status;
	uint8_t	sysmgr_storage_status;
	uint8_t sysmgr_sysmgr_status;
	uint8_t	sysmgr_utils_status;
	uint8_t	sysmgr_web_status;

} Web_driver_status_t;

typedef struct{

	uint32_t timestamp;

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

		uint8_t fix;
		uint8_t sats;
	} gps;

	float anglex;
	float angley;
	float anglez;

	float rake;
} Web_driver_live_t;


char* Web_driver_json_statusCreate(Web_driver_status_t status);
char* Web_driver_json_liveCreate(Web_driver_live_t status);
Web_driver_status_t Web_driver_json_parse(char* json);
