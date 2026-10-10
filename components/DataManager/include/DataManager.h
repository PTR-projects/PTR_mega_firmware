#pragma once

#include "IGN_driver.h"
#include "Sensors.h"
#include "Servo_driver.h"
#include "GNSS_driver.h"
#include "Analog_driver.h"
#include "AHRS_driver.h"
#include "FlightStateDetector.h"
#include "Servo_driver.h"
#include "PTR_DataPacket.h"

/**
 * @brief Data structure representing a data package.
 * A data package contains sensor readings, AHRS data, flight state information, and other data.
 *  !!! Any change to this structure force a change in data parser in Web_driver !!!
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

typedef struct {
	uint32_t timestamp_ms;
	Sensors_t sensors;
	gps_t gps;
	AHRS_t ahrs;
	flightstate_t flightstate;
	IGN_t ign;
	Analog_meas_t analog;
	servo_t servo;
} DataPackageWebLive_t;

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

void DM_collectRF(kppacket_t * package, int64_t time_us, Sensors_t * sensors, gps_t * gps, AHRS_t * ahrs, flightstate_t flightstate, IGN_t * ign, Analog_meas_t * analog);
