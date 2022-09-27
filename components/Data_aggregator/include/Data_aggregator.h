#pragma once

#include "IGN_driver.h"
#include "Sensors.h"
#include "Servo_driver.h"
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

	struct{
		float servo_1;
		float servo_2;
		float servo_3;
		float servo_4;
		uint8_t servo_en;
	} servo;
} DataPackage_t;

void Data_aggregate(DataPackage_t * package, int64_t time_us, Sensors_t * sensors, AHRS_t * ahrs, FlightState_t * flightstate, IGN_t * ign);
