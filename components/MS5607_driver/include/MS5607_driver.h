/**
 * @file MS5607.h
 * @brief Header file for the MS5607 barometric pressure sensor
 *
 * This file contains the declarations for the MS5607 barometric pressure sensor,
 * including the data structures for the sensor readings and calibration data,
 * as well as the function prototypes for initializing and reading from the sensor.
 *
 * @author John Doe (johndoe@example.com)
 * @date 2022-12-15
 */

#pragma once

#include "esp_err.h"

/**
 * @brief Data structure for a single measurement from the MS5607 sensor
 *
 * This structure holds the temperature and pressure readings from the MS5607
 * sensor in the `temp` and `press` fields, respectively. Both readings are
 * in degrees Celsius and hectopascals (hPa), respectively.
 */
typedef struct {
    float temp;   /**< Temperature reading in degrees Celsius */
    float press;  /**< Pressure reading in hectopascals (hPa) */
} MS5607_meas_t;

/**
 * @brief Data structure for the MS5607 sensor
 *
 * This structure holds the raw sensor readings (D1, D2), as well as
 * intermediate calculations used in the conversion from raw readings
 * to temperature and pressure readings. The final temperature and
 * pressure readings are stored in the `meas` field.
 */
typedef struct {
    uint32_t D1;          /**< Raw temperature reading */
    uint32_t D2;          /**< Raw pressure reading */
    int32_t dT;           /**< Intermediate calculation for temperature conversion */
    int64_t SENS2;        /**< Intermediate calculation for pressure conversion */
    int64_t OFF2;         /**< Intermediate calculation for pressure conversion */

    MS5607_meas_t meas;   /**< Final temperature and pressure readings */
} MS5607_t;

/**
 * @brief Data structure for the MS5607 sensor's calibration data
 *
 * This structure holds the calibration coefficients (C1, C2, C3, C4, C5, C6)
 * that are specific to each individual MS5607 sensor. These coefficients
 * are used in the conversion from raw readings to temperature and pressure
 * readings.
 */
typedef struct {
    uint16_t C1;  /**< Calibration coefficient C1 */
    uint16_t C2;  /**< Calibration coefficient C2 */
    uint16_t C3;  /**< Calibration coefficient C3 */
    uint16_t C4;  /**< Calibration coefficient C4 */
    uint16_t C5;  /**< Calibration coefficient C5 */
    uint16_t C6;  /**< Calibration coefficient C6 */
} MS5607_cal_t;



/**
 * @brief Initializes the MS5607 sensor
 *
 * This function initializes the MS5607 sensor by reading the calibration data
 * and performing an initial temperature and pressure measurement. This must
 * be called before any other functions in this module.
 *
 * @return
 * - ESP_OK if the initialization was successful
 * - ESP_FAIL if there was an error initializing the sensor
 */
esp_err_t MS5607_init();

/**
 * @brief Reads the temperature and pressure from the MS5607 sensor
 *
 * This function reads the temperature and pressure from the MS5607 sensor and
 * calculates the final temperature and pressure readings. It uses an algorithm
 * that takes into account the previous readings from the sensor in order to
 * provide more accurate results.
 *
 * @return
 * - ESP_OK if the readings were successful
 * - ESP_FAIL if there was an error reading from the sensor
 */
esp_err_t MS5607_getReloadSmart();

/**
 * @brief Returns the last measured pressure from the MS5607 sensor
 *
 * This function returns the last measured pressure from the MS5607 sensor in
 * hectopascals (hPa). It does not perform a new measurement.
 *
 * @return The last measured pressure in Pa
 */
float MS5607_getPress();

/**
 * @brief Returns the last measured temperature from the MS5607 sensor
 *
 * This function returns the last measured temperature from the MS5607 sensor in
 * degrees Celsius. It does not perform a new measurement.
 *
 * @return The last measured temperature in degrees Celsius
 */
float MS5607_getTemp();

/**
 * @brief Returns the last measured temperature and pressure from the MS5607 sensor
 *
 * This function returns the last measured temperature and pressure from the
 * MS5607 sensor in the `meas` parameter. The temperature is returned in degrees
 * Celsius, and the pressure is returned in pascals (Pa). It does not
 * perform a new measurement.
 *
 * @param[out] meas Pointer to an `MS5607_meas_t` structure where the temperature
 * and pressure readings will be stored
 *
 * @return
 * - ESP_OK if the readings were successful
 * - ESP_FAIL if there was an error reading from the sensor
 */
esp_err_t MS5607_getMeas(MS5607_meas_t * meas);


/*
 * Registers
 */
#define MS5607_ADC_READ 		0x00
#define MS5607_PROM_READ		0xA0
#define MS5607_RESET			0x1E


/*
 * Digital pressure oversampling settings
 */
#define MS5607_CONVERT_D1_256 	0x40
#define MS5607_CONVERT_D1_512 	0x42
#define MS5607_CONVERT_D1_1024 	0x44
#define MS5607_CONVERT_D1_2048 	0x46
#define MS5607_CONVERT_D1_4096	0x48


/*
 * Digital temperature oversampling settings
 */
#define MS5607_CONVERT_D2_256 	0x50
#define MS5607_CONVERT_D2_512 	0x52
#define MS5607_CONVERT_D2_1024 	0x54
#define MS5607_CONVERT_D2_2048 	0x56
#define MS5607_CONVERT_D2_4096	0x58
