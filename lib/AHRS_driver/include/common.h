#ifndef COMPONENTS_AHRS_DRIVER_INCLUDE_COMMON_H_
#define COMPONENTS_AHRS_DRIVER_INCLUDE_COMMON_H_

//-------------- Helpers for ahrs computation ---------------------
// https://github.com/betaflight/betaflight/blob/master/src/main/flight/imu.h
// https://github.com/betaflight/betaflight/blob/master/src/main/common/maths.h

#define POW2(x) ((x)*(x))
#define POW3(x) ((x)*(x)*(x))
#define POW5(x) ((x)*(x)*(x)*(x)*(x))
#define DEGREES_TO_RADIANS(angle) ((angle) * 0.0174532925f)
#define RADIANS_TO_DEGREES(rad) ((rad) * 57.29578049f)
#define EULER_INITIALIZE  { { 0, 0, 0 } }
#define HZ_TO_INTERVAL(x) (1.0f / (x))
#define HZ_TO_INTERVAL_US(x) (1000000 / (x))
#define M_PIf       3.14159265358979323846f	// Use floating point M_PI instead explicitly.
#define M_EULERf    2.71828182845904523536f


/**
 * @brief Union representing a 3D vector in two different forms: x, y, and z; or v[3].
 */
typedef union{
	struct{
		float x;
		float y;
		float z;
	};
	float v[3];
} vectorf_t;



#endif /* COMPONENTS_AHRS_DRIVER_INCLUDE_COMMON_H_ */
