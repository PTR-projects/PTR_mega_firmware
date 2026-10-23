#pragma once

/**
 * @file GNSS_types.h
 * @brief Plain data types shared by the GNSS driver, its backends and its consumers.
 *
 * This header intentionally has no ESP-IDF / FreeRTOS dependencies so that the protocol
 * parsers can be unit-tested on a host machine. The layout of gps_t must not depend on any
 * CONFIG_* option, otherwise translation units that include it in a different order would
 * disagree on the structure layout.
 */

#include <stdint.h>
#include <stdbool.h>

#define GPS_MAX_SATELLITES_IN_USE (12)
#define GPS_MAX_SATELLITES_IN_VIEW (16)
#define YEAR_BASE (2000) //date in NMEA starts from 2000

/**
 * @brief Navigation (dynamic platform) mode
 */
typedef enum {
	GPS_MODE_NORMAL = 0,
	GPS_MODE_FITNESS = 1,
	GPS_MODE_AVIATION = 2,
	GPS_MODE_BALLON = 3,
	GPS_MODE_STATIONARY = 4
} gps_nav_mode_t;

/**
 * @brief Type of the GNSS receiver / protocol in use
 */
typedef enum {
	GNSS_TYPE_AUTO = 0,    /*!< Detect receiver at start-up */
	GNSS_TYPE_NMEA_MTK,    /*!< NMEA protocol, MediaTek (PMTK) configuration commands */
	GNSS_TYPE_UBLOX_UBX    /*!< u-blox receiver (MAX-M10N), UBX protocol */
} gnss_type_t;

/**
 * @brief GPS fix type
 */
typedef enum {
	GPS_FIX_INVALID, /*!< Not fixed */
	GPS_FIX_GPS,     /*!< GPS */
	GPS_FIX_DGPS,    /*!< Differential GPS */
} gps_fix_t;

/**
 * @brief GPS fix mode
 */
typedef enum {
	GPS_MODE_INVALID = 1, /*!< Not fixed */
	GPS_MODE_2D,          /*!< 2D GPS */
	GPS_MODE_3D           /*!< 3D GPS */
} gps_fix_mode_t;

/**
 * @brief GPS time (UTC)
 */
typedef struct {
	uint8_t hour;      /*!< Hour */
	uint8_t minute;    /*!< Minute */
	uint8_t second;    /*!< Second */
	uint16_t thousand; /*!< Milliseconds */
} gps_time_t;

/**
 * @brief GPS date
 */
typedef struct {
	uint8_t day;   /*!< Day (start from 1) */
	uint8_t month; /*!< Month (start from 1) */
	uint16_t year; /*!< Full year, e.g. 2026 */
} gps_date_t;

/**
 * @brief GNSS solution. Fields a receiver/protocol cannot provide are left at 0
 *        (accuracy and DOP values: 0 = unknown).
 */
typedef struct {
	double latitude;            /*!< Latitude (degrees, + north) */
	double longitude;           /*!< Longitude (degrees, + east) */
	float altitude;             /*!< Altitude above mean sea level (meters) */
	float altitude_ellipsoid;   /*!< Height above WGS84 ellipsoid (meters) */

	gps_time_t tim;             /*!< Time in UTC */
	gps_date_t date;            /*!< Date in UTC */

	gps_fix_t fix;              /*!< Fix status */
	gps_fix_mode_t fix_mode;    /*!< Fix mode (0 if unknown) */
	bool valid;                 /*!< Receiver reports a valid fix */
	uint8_t sats_in_use;        /*!< Number of satellites used in solution */

	float dop_h;                /*!< Horizontal dilution of precision */
	float dop_p;                /*!< Position dilution of precision */
	float dop_v;                /*!< Vertical dilution of precision */

	float speed;                /*!< Ground speed (m/s) */
	float cog;                  /*!< Course over ground (degrees) */
	float variation;            /*!< Magnetic variation (degrees) */
	float vel_n;                /*!< North velocity (m/s) */
	float vel_e;                /*!< East velocity (m/s) */
	float vel_d;                /*!< Down velocity (m/s) */

	float h_acc;                /*!< Horizontal accuracy estimate (m), 0 = unknown */
	float v_acc;                /*!< Vertical accuracy estimate (m), 0 = unknown */
	float s_acc;                /*!< Speed accuracy estimate (m/s), 0 = unknown */

	uint32_t itow_ms;           /*!< GPS time of week of the solution (ms), UBX only */
	int64_t rx_time_us;         /*!< esp_timer time when the solution was completed */
	uint8_t source;             /*!< gnss_type_t of the backend that produced the data */
} gps_t;
