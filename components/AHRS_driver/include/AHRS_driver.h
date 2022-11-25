#pragma once

// TODO for future EKF3 - https://github.com/ArduPilot/ardupilot/blob/master/Tools/CPUInfo/EKF_Maths.h

#include "common.h"
#include "quaternion.h"

typedef union {
	struct{
		float roll;
		float pitch;
		float yaw;
	};
	struct{
		float tilt;
		float dir;
		float rot;
	};
	float raw[3];
} EulerAngle_t;

typedef struct{
	float rMat[3][3];
	quaternions_t quaternions;
	EulerAngle_t euler;
} orientation_t;

typedef struct{
	float acc_up;
	float ascent_rate;
	float altitude;

	vectorf_t acc_rf;

	orientation_t orientation;

	float max_altitude;
	float acc_axis_lowpass;

	float altitudeP;
	float velocityP;

	uint64_t prev_time_us;
	float dt;
} AHRS_t;


esp_err_t AHRS_init(int64_t time_us);
AHRS_t * AHRS_getData();
esp_err_t AHRS_compute(int64_t time_us, Sensors_t * sensors);
void AHRS_UpdateReferencePressure(float press);
