#pragma once

#include "IGN_driver.h"
#include "Sensors.h"
#include "Servo_driver.h"
#include "GNSS_driver.h"
#include "Analog_driver.h"
#include "AHRS_driver.h"
#include "FlightStateDetector.h"

typedef struct{
	uint64_t sys_time;
	struct{
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
		float temp;

		float latitude;
		float longitude;
		float altitude_gnss;
		int8_t gnss_fix;
	} sensors;

	struct{
		float altitude_press;

		float posX;
		float posY;
		float posZ;

		float veloX;
		float veloY;
		float veloZ;

		float q1, q2, q3, q4;
	} ahrs;

	uint8_t flightstate;

	struct{
		uint8_t ign1_cont : 1;
		uint8_t ign2_cont : 1;
		uint8_t ign3_cont : 1;
		uint8_t ign4_cont : 1;

		uint8_t ign1_state : 1;
		uint8_t ign2_state : 1;
		uint8_t ign3_state : 1;
		uint8_t ign4_state : 1;
	} ign;

	uint32_t vbat_mV;

	struct{
		float servo_1;
		float servo_2;
		float servo_3;
		float servo_4;
		uint8_t servo_en;
	} servo;
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

void Data_aggregate(DataPackage_t * package, int64_t time_us, Sensors_t * sensors, gps_t * gps, AHRS_t * ahrs, FlightState_t * flightstate, IGN_t * ign, Analog_meas_t * analog);
void Data_aggregateRF(DataPackageRF_t * package, int64_t time_us, Sensors_t * sensors, gps_t * gps, AHRS_t * ahrs, FlightState_t * flightstate, IGN_t * ign);
