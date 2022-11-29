#pragma once

// TODO for future EKF3 - https://github.com/ArduPilot/ardupilot/blob/master/Tools/CPUInfo/EKF_Maths.h

typedef union{
	struct{
		float x;
		float y;
		float z;
	};
	float v[3];
} vectorf_t;

typedef union {
	struct{
		float w;
		float x;
		float y;
		float z;
	};
	struct{
		float q0;
		float q1;
		float q2;
		float q3;
	};
} quaternions_t;

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
	vectorf_t acc_ef;
	vectorf_t vel_ef;
	vectorf_t pos_ef;

	vectorf_t acc_rf;
	vectorf_t vel_rf;

	orientation_t orientation;

	float max_altitude;
	float acc_axis_lowpass;

} AHRS_t;


//-------------- Helpers for ahrs computation ---------------------
// https://github.com/betaflight/betaflight/blob/master/src/main/flight/imu.h
// https://github.com/betaflight/betaflight/blob/master/src/main/common/maths.h
#define POWER2(x) ((x)*(x))
#define POWER3(x) ((x)*(x)*(x))
#define POWER5(x) ((x)*(x)*(x)*(x)*(x))
#define DEGREES_TO_RADIANS(angle) ((angle) * 0.0174532925f)
#define QUATERNION_INITIALIZE  {.w=1, .x=0, .y=0,.z=0}
#define QUATERNION_PRODUCTS_INITIALIZE  {.ww=1, .wx=0, .wy=0, .wz=0, .xx=0, .xy=0, .xz=0, .yy=0, .yz=0, .zz=0}
#define EULER_INITIALIZE  { { 0, 0, 0 } }
#define HZ_TO_INTERVAL(x) (1.0f / (x))
#define HZ_TO_INTERVAL_US(x) (1000000 / (x))
#define M_PIf       3.14159265358979323846f	// Use floating point M_PI instead explicitly.
#define M_EULERf    2.71828182845904523536f

typedef struct {
	float ww;
	float wx;
	float wy;
	float wz;
	float xx;
	float xy;
	float xz;
	float yy;
	float yz;
	float zz;
} quaternionsProd_t;
