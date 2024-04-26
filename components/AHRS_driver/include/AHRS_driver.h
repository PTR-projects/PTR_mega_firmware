#pragma once

// TODO for future EKF3 - https://github.com/ArduPilot/ardupilot/blob/master/Tools/CPUInfo/EKF_Maths.h

#include "common.h"
#include "quaternion.h"
#include "Sensors.h"


/**
 * @brief Data union representing a set of Euler angles.
 * Euler angles represent the orientation of a device in 3D space.
 * They can be represented in different conventions (roll-pitch-yaw or tilt-dir-rot).
 */
typedef union {
	struct{
		float roll;		/*!< Roll angle (in degrees). */
		float pitch;	/*!< Pitch angle (in degrees). */
		float yaw;		/*!< Yaw angle (in degrees). */
	};
	struct{
		float tilt;		/*!< Tilt angle from vertical (in degrees). */
		float dir;		/*!< Direction angle (in degree). */
		float rot;		/*!< Rotation angle (in degree). */
	};
	float raw[3];		/*!< Raw data. */
} EulerAngle_t;


/**
 * @brief Structure representing orientation data in three different forms: rotation matrix, quaternions, or Euler angles.
 */
typedef struct{
	float rMat[3][3];				/*!< Rotation matrix representing the orientation. */
	quaternions_t quaternions;		/*!< Quaternion representation of the orientation. */
	EulerAngle_t euler;				/*!< Euler angles representation of the orientation. */
} orientation_t;


/**
 * @brief Data structure representing an Attitude and Heading Reference System (AHRS).
 */
typedef struct{
	float acc_up;					/*!< Acceleration in the upward direction. */
	float ascent_rate;				/*!< Ascent rate (vertical velocity). [m/s] */
	float altitude;					/*!< Altitude above a reference point. [m] */

	vectorf_t acc_rf;				/*!< Acceleration in the rocket frame. */

	orientation_t orientation;		/*!< Orientation of the device. */

	float max_altitude;				/*!< Maximum altitude reached. [m] */
	float acc_axis_lowpass;			/*!< Low-pass filtered acceleration on the three axes. */

	float altitudeP;				/*!< Altitude above a reference point calculated from pressure. [m] */
	float velocityP;				/*!< Vertical velocity calculated from pressure. [m/s] */

	uint64_t prev_time_us;			/*!< Previous time stamp (in microseconds). */
	float dt;						/*!< Time step (in seconds). */
} AHRS_t;

/**
 * @brief Initializes the AHRS (Attitude and Heading Reference System) module with the specified time.
 * @param[in] time_us The time in microseconds.
 * @return ESP_OK if initialization was successful, ESP_FAIL otherwise.
 */
esp_err_t AHRS_init(int64_t time_us);

/**
 * @brief Gets the current data from the AHRS module.
 * @return A pointer to the AHRS data structure.
 */
AHRS_t * AHRS_getData();

/**
 * @brief Computes the attitude and heading using the specified time and sensor data.
 * @param[in] time_us The time in microseconds.
 * @param[in] sensors A pointer to the sensor data structure.
 * @return ESP_OK if computation was successful, ESP_FAIL otherwise.
 */
esp_err_t AHRS_compute(int64_t time_us, Sensors_t * sensors);

/**
 * @brief Configures the orientation settings for the AHRS module.
 * @param[in] enableAcc True to enable acceleration data for orientation calculations, false to disable.
 * @param[in] enableMag True to enable magnetometer data for orientation calculations, false to disable.
 */
void AHRS_orientationSettings(uint8_t enableAcc, uint8_t enableMag);

/**
 * @brief TODO
 */
void AHRS_setInFlight();
