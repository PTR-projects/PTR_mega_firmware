/*
 * KF_AltitudeAscent.h
 *
 *  Created on: 26 lis 2022
 *      Author: bartek
 */

#ifndef COMPONENTS_AHRS_DRIVER_INCLUDE_KF_ALTITUDEASCENT_H_
#define COMPONENTS_AHRS_DRIVER_INCLUDE_KF_ALTITUDEASCENT_H_


/**
 * @brief Structure representing the variables for a Kalman filter used for altitude ascent estimation.
 */
typedef struct {
	float h;
	float v;
	float P[2][2];
	float Q_accel;
	float R_altitude;
} KF_AltitudeAscent_t;

/**
 * @brief Initializes the Kalman filter for altitude ascent estimation.
 * @param[in] Q_accel The process noise variance for acceleration.
 * @param[in] R_altitude The measurement noise variance for altitude.
 */
void AHRS_kalmanAltitudeAscent_init(float Q_accel, float R_altitude);


/**
 * @brief Performs a step in the Kalman filter for altitude ascent estimation.
 * @param[in] dt The time step.
 * @param[in] altitudeP The measured altitude.
 * @param[in] accZ The measured acceleration in the Z axis.
 * @param[out] altitude_result The estimated altitude.
 * @param[out] ascentrate_result The estimated vertical velocity.
 */
void AHRS_kalmanAltitudeAscent_step(float dt, float altitudeP, float accZ, float * altitude_result, float * ascentrate_result);

#endif /* COMPONENTS_AHRS_DRIVER_INCLUDE_KF_ALTITUDEASCENT_H_ */
