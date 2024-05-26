#pragma once

#include "IGN_driver.h"
#include "Sensors.h"
#include "Servo_driver.h"
#include "GNSS_driver.h"
#include "Analog_driver.h"
#include "AHRS_driver.h"
#include "FlightStateDetector.h"
#include "Servo_driver.h"

/**
 * @brief Data structure representing a data package.
 * A data package contains sensor readings, AHRS data, flight state information, and other data.
 */
typedef struct __attribute__((__packed__)){
	uint32_t sys_time;
	struct __attribute__((__packed__)){
		float accX;				/*!< Acceleration along the X-axis. */
		float accY;				/*!< Acceleration along the Y-axis. */
		float accZ;				/*!< Acceleration along the Z-axis. */

		float gyroX;			/*!< Angular velocity around the X-axis. */
		float gyroY;			/*!< Angular velocity around the Y-axis. */
		float gyroZ;			/*!< Angular velocity around the Z-axis. */

		float magX;				/*!< Magnetic field strength along the X-axis. */
		float magY;				/*!< Magnetic field strength along the Y-axis. */
		float magZ;				/*!< Magnetic field strength along the Z-axis. */

		float accHX;			/*!< Acceleration along the X-axis (high range). */
		float accHY;			/*!< Acceleration along the Y-axis (high range). */
		float accHZ;			/*!< Acceleration along the Z-axis (high range). */

		float pressure;			/*!< Barometric pressure. */
		int8_t temp;			/*!< Temperature. */

		float latitude;			/*!< Latitude (in degrees). */
		float longitude;		/*!< Longitude (in degrees). */
		float altitude_gnss;	/*!< Altitude from GNSS (Global Navigation Satellite System). */
		int8_t gnss_fix;		/*!< GNSS fix status (0 = no fix, 1 = fix). */
	} sensors;					/*!< Sensor readings. */

	struct __attribute__((__packed__)){
		float altitude_press;		/*!< Altitude derived from pressure. */
		float altitude_kalman;		/*!< Altitude using Kalman filtering. */
		float ascent_rate_kalman;	/*!< TODO */
		uint8_t tilt;

		float q0, q1, q2, q3;		/*!< Quaternions.  */
	} ahrs;							/*!< Attitude and heading reference system (AHRS) data. */

	uint8_t flightstate;	/*!< Flight state. */

	struct __attribute__((__packed__)){
		uint8_t ign1_cont : 1;			/*!< IGN1 continuity flag. */
		uint8_t ign2_cont : 1;			/*!< IGN2 continuity flag. */
		uint8_t ign3_cont : 1;			/*!< IGN3 continuity flag. */
		uint8_t ign4_cont : 1;			/*!< IGN4 continuity flag. */

		uint8_t ign1_state : 1;			/*!< IGN1 state flag. */
		uint8_t ign2_state : 1;			/*!< IGN2 state flag. */
		uint8_t ign3_state : 1;			/*!< IGN3 state flag. */
		uint8_t ign4_state : 1;			/*!< IGN4 state flag. */
	} ign;								/*!< Ignition system information. */

	uint16_t vbat_mV;	/*!< Battery voltage (in millivolts). */

	struct __attribute__((__packed__)){
		int8_t servo_1;			/*!< Position of servo 1 (in %). */
		int8_t servo_2;			/*!< Position of servo 2 (in %). */
		int8_t servo_3;			/*!< Position of servo 3 (in %). */
		int8_t servo_4;			/*!< Position of servo 4 (in %). */
		uint8_t servo_en;		/*!< Servo enable flag. */
	} servo;					/*!< Servo status information. */

	uint8_t blank[4];
} DataPackage_t;

/**
 * @brief Data structure representing a data package for radio frequency (RF) transmission.
 * This data structure is packed to reduce the size of the transmitted data.
 */
typedef struct __attribute__((__packed__)){
	uint16_t packet_id;				/*!< Packet identifier. */
	uint16_t id;					/*!< Device identifier. */
	uint16_t packet_no;				/*!< Packet number. */
	uint32_t timestamp_ms;			/*!< Timestamp (in milliseconds). */
	uint8_t state;					/*!< Device state. */
	uint8_t flags;					/*!< Flags. */

	uint8_t vbat_10;				/*!< Battery voltage (in decivolts [V*10]). *///

	int16_t accX_100;				/*!< Acceleration on the X axis (in hundredths of g [G*100]). */
	int16_t accY_100;				/*!< Acceleration on the Y axis (in hundredths of g [G*100]). */
	int16_t accZ_100;				/*!< Acceleration on the Z axis (in hundredths of g [G*100]). */

	int16_t gyroX_10;				/*!< Angular velocity on the X axis (in tenths of degrees per second [deg/s * 10]). */ //Check unit
	int16_t gyroY_10;				/*!< Angular velocity on the Y axis (in tenths of degrees per second [deg/s  * 10]). */
	int16_t gyroZ_10;				/*!< Angular velocity on the Z axis (in tenths of degrees per second [deg/s  * 10]). */

	int16_t tilt_100;				/*!< Tilt angle (in hundredths of degrees [deg*100]). */
	float pressure;					/*!< Pressure (in Pascals). */
	int16_t velocity_10;			/*!< Velocity (in tenths of meters per second [m/s*10]). */
	uint16_t altitude;				/*!< Altitude in emters [m]. */

	int32_t lat;		/*!< Latitude (in 1e-7 degrees). */
	int32_t lon;		/*!< Longitude (in 1e-7 degrees). */
	int32_t alti_gps;	/*!< Altitude above the ellipsoid (in millimeters). */
	uint8_t sats_fix;	/*!< Number of satellites and fix status (6b sats + 2b fix). */
} DataPackageRF_t;

/**
 * @brief Initialize the data manager (DM) module.
 * @return ESP_OK if initialization was successful, ESP_FAIL otherwise.
 */
esp_err_t DM_init();

uint16_t DM_checkWaitingElementsNumber();

/**
 * @brief Get a used data pointer from the main ring buffer (RB).
 * @param[out] ptr Pointer to a ::DataPackage_t pointer where the address of the used data will be stored.
 * @return ESP_OK if a used data pointer was obtained, ESP_FAIL otherwise.
 */
esp_err_t DM_getUsedPointerFromMainRB(DataPackage_t ** ptr);

/**
 * @brief Get a used data pointer from the main ring buffer (RB), with wait.
 * This function will wait until a used data pointer is available.
 * @param[out] ptr Pointer to a ::DataPackage_t pointer where the address of the used data will be stored.
 * @return ESP_OK if a used data pointer was obtained, ESP_FAIL otherwise.
 */
esp_err_t DM_getUsedPointerFromMainRB_wait(DataPackage_t ** ptr);

/**
 * @brief Return a used data pointer to the main ring buffer (RB).
 * @param[in] ptr Pointer to a ::DataPackage_t pointer containing the address of the used data.
 * @return ESP_OK if the used data pointer was returned, ESP_FAIL otherwise.
 */
esp_err_t DM_returnUsedPointerToMainRB(DataPackage_t ** ptr);

/**
 * @brief Get a free data pointer from the main ring buffer (RB).
 * @param[out] ptr Pointer to a ::DataPackage_t pointer where the address of the free data will be stored.
 * @return ESP_OK if a free data pointer was obtained, ESP_FAIL otherwise.
 */
esp_err_t DM_getFreePointerToMainRB(DataPackage_t ** ptr);

/**
 * @brief Add a data pointer to the main ring buffer (RB).
 * @param[in] ptr Pointer to a ::DataPackage_t pointer containing the address of the data to be added.
 * @return ESP_OK if the data pointer was added, ESP_FAIL otherwise.
 */
esp_err_t DM_addToMainRB(DataPackage_t ** ptr);

/**
 * @brief Collect data for storage in flash memory.
 * @param[out] package Pointer to a ::DataPackage_t structure where the collected data will be stored.
 * @param[in] time_us Timestamp (in microseconds).
 * @param[in] sensors Pointer to a ::Sensors_t structure containing sensor data.
 * @param[in] gps Pointer to a ::gps_t structure containing GPS data.
 * @param[in] ahrs Pointer to an ::AHRS_t structure containing attitude and heading reference system (AHRS) data.
 * @param[in] flightstate Pointer to a ::FlightState_t structure containing flight state data.
 * @param[in] ign Pointer to an ::IGN_t structure containing ignition data.
 * @param[in] analog Pointer to an ::Analog_meas_t structure containing analog measurement data.
 */
void DM_collectFlash(DataPackage_t * package, int64_t time_us, Sensors_t * sensors, gps_t * gps, AHRS_t * ahrs, flightstate_t flightstate, IGN_t * ign, Analog_meas_t * analog, servo_t * servo);

/**
 * @brief Collect data from various sources and store them in a data package for RF transmission.
 * @param[out] package Pointer to a ::DataPackageRF_t structure where the collected data will be stored.
 * @param[in] time_us Timestamp (in microseconds).
 * @param[in] sensors Pointer to a ::Sensors_t structure containing sensor data.
 * @param[in] gps Pointer to a ::gps_t structure containing GPS data.
 * @param[in] ahrs Pointer to an ::AHRS_t structure containing AHRS data.
 * @param[in] flightstate Pointer to a ::FlightState_t structure containing flight state data.
 * @param[in] ign Pointer to an ::IGN_t structure containing IGN data.
 */

void DM_collectRF(DataPackageRF_t * package, int64_t time_us, Sensors_t * sensors, gps_t * gps, AHRS_t * ahrs, flightstate_t flightstate, IGN_t * ign);
