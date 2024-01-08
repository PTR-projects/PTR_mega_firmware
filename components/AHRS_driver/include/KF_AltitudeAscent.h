/*
 * KF_AltitudeAscent.h
 *
 *  Created on: 26 lis 2022
 *      Author: bartek
 */

#ifndef COMPONENTS_AHRS_DRIVER_INCLUDE_KF_ALTITUDEASCENT_H_
#define COMPONENTS_AHRS_DRIVER_INCLUDE_KF_ALTITUDEASCENT_H_

typedef struct {
	float h;
	float v;
	float P[2][2];
	float Q_accel;
	float R_altitude;
} KF_AltitudeAscent_t;

void AHRS_kalmanAltitudeAscent_init(float Q_accel, float R_altitude);
void AHRS_kalmanAltitudeAscent_step(float dt, float altitudeP, float acc_up, float * altitude_result, float * ascentrate_result);

#endif /* COMPONENTS_AHRS_DRIVER_INCLUDE_KF_ALTITUDEASCENT_H_ */
