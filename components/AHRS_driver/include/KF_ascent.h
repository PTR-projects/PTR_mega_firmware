#pragma once

#include "matrix.h"


/**
 * @brief Structure representing the variables for a Kalman filter used for altitude ascent estimation.
 */
typedef struct {
	Matrix X;
	Matrix P;
	Matrix F;
	Matrix G;
	Matrix Q;

	Matrix R_GNSS;
	Matrix R_BARO;

	Matrix H_GNSS;
	Matrix H_BARO;

	
	
} KF_ascent_t;


/**
 * @brief Initializes the Kalman filter for altitude ascent estimation.
 * @param[in] Q_accel The process noise variance for acceleration.
 * @param[in] R_altitude The measurement noise variance for altitude.
 */
void AHRS_AHRS_KF_ascent_init();


void AHRS_KF_ascent_step(float dt, float acc_up, float * altitude_result, float * ascentrate_result);
void  AHRS_KF_ascent_update_BARO	(float altitude_BARO);
void  AHRS_KF_ascent_update_GNSS	(float altitude_GNSS, float dop);
