#pragma once

#include "driver/uart.h"
#include "esp_event.h"
#include "GNSS_types.h"

/**
 * @brief Initialize GNSS driver: detect receiver (if type is AUTO), configure it and start the parser task
 *
 * Detects the receiver automatically (GNSS_TYPE_AUTO). Safe to call again after a failure; returns ESP_OK immediately
 * if the driver is already running.
 *
 * @return esp_err_t
 *  - ESP_OK: Success
 *  - ESP_FAIL: Fail
 */
esp_err_t GPS_init();

/**
 * @brief Same as GPS_init() but with an explicit receiver type
 *
 * @param type GNSS_TYPE_AUTO, GNSS_TYPE_NMEA_MTK or GNSS_TYPE_UBLOX_UBX
 */
esp_err_t GPS_init_type(gnss_type_t type);

/**
 * @brief Receiver type in use (never GNSS_TYPE_AUTO after a successful init)
 */
gnss_type_t GPS_getType(void);

/**
 * @brief Check size of GPS message received
 *
 * @return uint8_t size of GPS message struct
 */
uint8_t GNSS_message_size(void);

/**
 * @brief Get GPS positional data
 *
 * @param data received data container
 * @param ms waiting for receive timeout
 * @return uint32_t size of received data (0 if nothing was received)
 */
uint32_t GPS_getData(gps_t * data, uint16_t ms);

/**
 * @brief Check if a complete GNSS solution was received recently
 *
 * @return ESP_OK if the last solution is younger than 5 s, ESP_FAIL otherwise
 */
esp_err_t GPS_checkStatus();

/**
 * @brief Test if GPS is responding (MTK: query firmware release, u-blox: poll UBX-MON-VER)
 */
void GPS_test(void);

/**
 * @brief Set GPS baud rate (receiver only)
 *
 * @param baud desired baudrate: 4800, 9600, 14400, 19200, 38400, 57600, 115200
 * @return esp_err_t ESP_OK on success
 */
esp_err_t GPS_baud_rate_set(uint32_t baud);

/**
 * @brief Set GPS baud rate and ESP UART baudrate to same value
 *
 * @param baud desired baudrate: 4800, 9600, 14400, 19200, 38400, 57600, 115200
 * @return esp_err_t ESP_OK on success
 */
esp_err_t GPS_baud_rate_set_extra(uint32_t baud);

/**
 * @brief Set GPS fix interval
 *
 * @param time time in millis 100-10000 (NMEA/MTK) or 40-10000 (u-blox)
 */
void GPS_fix_interval_set(uint16_t time);

/**
 * @brief Set navigation mode for your specific use case
 *
 * @param mode mode of GPS to use
 */
void GPS_nav_mode_set(gps_nav_mode_t mode);

/**
 * @brief Set NMEA output rates (NMEA/MTK receivers only, ignored for u-blox/UBX)
 *
 * @param GLL Geographic position – latitude and longitude
 * @param RMC Recommended minimum specific GPS/Transit data
 * @param VTG Course Over Ground and Ground Speed
 * @param GGA Global Positioning System Fix Data
 * @param GSA GNSS DOP and Active Satellites
 * @param GSV GNSS Satellites in View
 */
void GPS_nmea_output_set(uint8_t GLL, uint8_t RMC, uint8_t VTG, uint8_t GGA, uint8_t GSA, uint8_t GSV);
