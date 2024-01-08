/*
 * quaternion.h
 *
 *  Created on: 24 lis 2022
 *      Author: bartek
 */

#ifndef COMPONENTS_AHRS_DRIVER_INCLUDE_QUATERNION_H_
#define COMPONENTS_AHRS_DRIVER_INCLUDE_QUATERNION_H_
#include "common.h"

// https://github.com/betaflight/betaflight/blob/master/src/main/common/maths.h
#define QUATERNION_INITIALIZE  {.w=1, .x=0, .y=0,.z=0}
#define QUATERNION_PRODUCTS_INITIALIZE  {.ww=1, .wx=0, .wy=0, .wz=0, .xx=0, .xy=0, .xz=0, .yy=0, .yz=0, .zz=0}

// https://github.com/iNavFlight/inav/blob/master/src/main/common/quaternion.h

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


void quaternionInitUnit			(quaternions_t * result);
void quaternionInitFromVector	(quaternions_t * result, quaternions_t * v);
void quaternionMultiply			(quaternions_t * result, quaternions_t * a, quaternions_t * b);
void quaternionScale			(quaternions_t * result, quaternions_t * a, float b);
void quaternionAdd				(quaternions_t * result, quaternions_t * a, quaternions_t * b);
void quaternionConjugate		(quaternions_t * result, quaternions_t * q);
void quaternionNormalize		(quaternions_t * result, quaternions_t * q);
void quaternionRotateVector		(vectorf_t * result,     vectorf_t * vect,  quaternions_t * ref);
void quaternionRotateVectorInv	(vectorf_t * result,     vectorf_t * vect,  quaternions_t * ref);
void quaternionComputeProducts	(quaternions_t *quat,    quaternionsProd_t *quatProd);

#endif /* COMPONENTS_AHRS_DRIVER_INCLUDE_QUATERNION_H_ */
