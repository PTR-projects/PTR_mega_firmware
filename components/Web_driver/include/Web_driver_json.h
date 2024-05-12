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

/**
* @brief Struct storing all the status data
*/
typedef struct{
	uint64_t serial_number;				/*!< Computer specific serial number */
	uint64_t software_version;			/*!< Current software version  */
	uint32_t timestamp_ms;				/*!< Current system time */
	uint8_t flight_state;				/*!< Current flight state */

	int drouge_alt;
	int main_alt;

	float battery_voltage;				/*!< Current battery voltage */
	float rocket_tilt;					/*!< Deviation from vertical axis relative to earth */


	/**
	* @brief Igniter information data
	*/
	struct {
		uint8_t fired;					/*!< Igniter ignition information */
		uint8_t continuity;				/*!< Ingiter continuity information */
	} igniters[4];

	float pressure;
	uint8_t gps_fix;
	uint8_t gps_sats;

	uint8_t sysmgr_system_status;
	uint8_t sysmgr_main_status;
	uint8_t sysmgr_analog_status;
	uint8_t	sysmgr_lora_status;
	uint8_t	sysmgr_adcs_status;
	uint8_t	sysmgr_storage_status;
	uint8_t sysmgr_sysmgr_status;
	uint8_t	sysmgr_utils_status;
	uint8_t	sysmgr_web_status;
	uint8_t sysmgr_arm_state;

} Web_driver_status_t;

typedef struct{

	uint32_t timestamp;				/*!< Current system time */

	/**
	* @brief MS5607 sensor data
	*/
	struct {
		float pressure;				/*!< Barometric pressure */
		float altitude;				/*!< Altitude AGL */
		float temperature;			/*!< Temperature */
	} MS5607;


	/**
	* @brief LIS331 high G accelerometer data
	*/
	struct {
		float ax;					/*!< X axis acceleration */
		float ay;					/*!< Y axis acceleration */
		float az;					/*!< Z axis acceleration */
	} LIS331;

	/**
	* @brief LSM6DS32 redundant IMU data
	*/
	struct {
		float ax;					/*!< X axis acceleration */
		float ay;					/*!< Y axis acceleration */
		float az;					/*!< Z axis acceleration */

		float gx;					/*!< X axis angular velocity */
		float gy;					/*!< Y axis angular velocity */
		float gz;					/*!< Z axis angular velocity */

		float temperature;
	} LSM6DS32_0, LSM6DS32_1;


	/**
	* @brief MMC magnetometer data
	*/
	struct {	
		float mx;					/*!< X axis magnetic flux */
		float my;					/*!< Y axis magnetic flux */
		float mz;					/*!< Z axis magnetic flux */
	} MMC5983MA;

	struct {
		float latitude;				/*!< Latitude */
		float longitude;			/*!< Longitude */

		uint8_t fix;				/*!< GPS fix check */
		uint8_t sats;				/*!< Number of satellites in view */
	} gps;

	float anglex;
	float angley;
	float anglez;

	float rake;
} Web_driver_live_t;

/**
* @brief Create and fill JSON string with current computer status data
* @param[in] Web_driver_status_t Current status
* @return char*
*/
char* Web_driver_json_statusCreate(Web_driver_status_t status);

/**
* @brief Create and fill JSON string with current computer data for live monitoring
* @param[in] Web_driver_status_t Current status
* @return char*
*/
char* Web_driver_json_liveCreate(Web_driver_live_t status);
Web_driver_status_t Web_driver_json_parse(char* json);
char* Web_driver_json_prefCreate();
