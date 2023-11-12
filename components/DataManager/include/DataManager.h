#pragma once

#include "IGN_driver.h"
#include "Sensors.h"
#include "Servo_driver.h"
#include "GNSS_driver.h"
#include "Analog_driver.h"
#include "AHRS_driver.h"
#include "FlightStateDetector.h"

typedef struct __attribute__((__packed__)){
	uint32_t sys_time;
	struct __attribute__((__packed__)){
		float accX;
		float accY;
		float accZ;

		float gyroX;
		float gyroY;
		float gyroZ;

		float magX;
		float magY;
		float magZ;

		float accHX;
		float accHY;
		float accHZ;

		float pressure;
		int8_t temp;

		float latitude;
		float longitude;
		float altitude_gnss;
		int8_t gnss_fix;
	} sensors;

	struct __attribute__((__packed__)){
		float altitude_press;
		float altitude_kalman;
		float ascent_rate_kalman;
		uint8_t tilt;

		float q0, q1, q2, q3;
	} ahrs;

	uint8_t flightstate;

	struct __attribute__((__packed__)){
		uint8_t ign1_cont : 1;
		uint8_t ign2_cont : 1;
		uint8_t ign3_cont : 1;
		uint8_t ign4_cont : 1;

		uint8_t ign1_state : 1;
		uint8_t ign2_state : 1;
		uint8_t ign3_state : 1;
		uint8_t ign4_state : 1;
	} ign;

	uint16_t vbat_mV;

	struct __attribute__((__packed__)){
		int8_t servo_1;
		int8_t servo_2;
		int8_t servo_3;
		int8_t servo_4;
		uint8_t servo_en;
	} servo;

	uint8_t blank[4];
} DataPackage_t;

typedef struct __attribute__((__packed__)){
	uint16_t packet_id;
	uint16_t id;
	uint16_t packet_no;
	uint32_t timestamp_ms;
	uint8_t state;
	uint8_t flags;

	uint8_t vbat_10;	//Vbat*10

	int16_t accX_100;	//Acc*100 [g]
	int16_t accY_100;
	int16_t accZ_100;

	int16_t gyroX_10;	//Gyro*10 [deg/s]
	int16_t gyroY_10;
	int16_t gyroZ_10;

	int16_t tilt_100;	//Tilt*100 [deg]
	float pressure;		//Pressure [Pa]
	int16_t velocity_10;	//Velocity*10 [m/s]
	uint16_t altitude;		//Altitude [m]

	int32_t lat;		//[1e-7 deg]
	int32_t lon;		//[1e-7 deg]
	int32_t alti_gps;	//Height above ellipsoid [- mm]
	uint8_t sats_fix;	//6b - sats + 2b fix
} DataPackageRF_t;

esp_err_t DM_init();
uint16_t DM_checkWaitingElementsNumber();
esp_err_t DM_getUsedPointerFromMainRB(DataPackage_t ** ptr);
esp_err_t DM_getUsedPointerFromMainRB_wait(DataPackage_t ** ptr);
esp_err_t DM_returnUsedPointerToMainRB(DataPackage_t ** ptr);
esp_err_t DM_getFreePointerToMainRB(DataPackage_t ** ptr);
esp_err_t DM_addToMainRB(DataPackage_t ** ptr);
void DM_collectFlash(DataPackage_t * package, int64_t time_us, Sensors_t * sensors, gps_t * gps, AHRS_t * ahrs, flightstate_t flightstate, IGN_t * ign, Analog_meas_t * analog);
void DM_collectRF(DataPackageRF_t * package, int64_t time_us, Sensors_t * sensors, gps_t * gps, AHRS_t * ahrs, flightstate_t flightstate, IGN_t * ign);
