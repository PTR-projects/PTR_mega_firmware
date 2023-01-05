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

/**
 * @brief Union representing quaternions in two different forms: w, x, y, and z; or q0, q1, q2, and q3.
 */
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

/**
 * @brief Structure representing the product of two quaternions.
 */
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

/**
 * @brief Initializes a quaternion to the unit quaternion.
 * @param[out] result The quaternion to initialize.
 */
void quaternionInitUnit(quaternions_t * result);

/**
 * @brief Initializes a quaternion from a vector.
 * @param[out] result The quaternion to initialize.
 * @param[in] v The vector to use for initialization.
 */
void quaternionInitFromVector(quaternions_t * result, quaternions_t * v);

/**
 * @brief Multiplies two quaternions.
 * @param[out] result The result of the multiplication.
 * @param[in] a The first quaternion.
 * @param[in] b The second quaternion.
 */
void quaternionMultiply(quaternions_t * result, quaternions_t * a, quaternions_t * b);

/**
 * @brief Scales a quaternion by a scalar value.
 * @param[out] result The resulting quaternion.
 * @param[in] a The quaternion to scale.
 * @param[in] b The scalar value.
 */
void quaternionScale(quaternions_t * result, quaternions_t * a, float b);

/**
 * @brief Adds two quaternions.
 * @param[out] result The resulting quaternion.
 * @param[in] a The first quaternion.
 * @param[in] b The second quaternion.
 */
void quaternionAdd(quaternions_t * result, quaternions_t * a, quaternions_t * b);

/**
 * @brief Calculates the conjugate of a quaternion.
 * @param[out] result The resulting quaternion.
 * @param[in] q The quaternion to calculate the conjugate of.
 */
void quaternionConjugate(quaternions_t * result, quaternions_t * q);

/**
 * @brief Normalizes a quaternion.
 * @param[out] result The resulting quaternion.
 * @param[in] q The quaternion to normalize.
 */
void quaternionNormalize(quaternions_t * result, quaternions_t * q);

/**
 * @brief Rotates a vector using a quaternion.
 * @param[out] result The resulting rotated vector.
 * @param[in] vect The vector to rotate.
 * @param[in] ref The quaternion to use for rotation.
 */
void quaternionRotateVector(vectorf_t * result, quaternions_t * vect, quaternions_t * ref);

/**
 * @brief Rotates a vector using the inverse of a quaternion.
 * @param[out] result The resulting rotated vector.
 * @param[in] vect The vector to rotate.
 * @param[in] ref The quaternion to use for rotation.
 */
void quaternionRotateVectorInv(vectorf_t * result, vectorf_t * vect, quaternions_t * ref);

/**
 * @brief Calculates the products of a quaternion.
 * @param[in] quat The quaternion to calculate the products for.
 * @param[out] quatProd The resulting products of the quaternion.
 */
void quaternionComputeProducts(quaternions_t *quat, quaternionsProd_t *quatProd);

#endif /* COMPONENTS_AHRS_DRIVER_INCLUDE_QUATERNION_H_ */
