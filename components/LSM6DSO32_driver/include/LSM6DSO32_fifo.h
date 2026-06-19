/**
 * @file LSM6DSO32_fifo.h
 * @brief Header file containing functions for configuring and reading FIFO data from LSM6DSO32 sensor.
 */

#ifndef LSM6DSO32_FIFO_H
#define LSM6DSO32_FIFO_H
#include <LSM6DSO32_private.h>

/**
 * @brief LSM6DSO32 fifoBuffer.
 */
#define LSM6DS_FIFO_BATCH_SIZE 16 //To be moved to config

/**
 * @brief Structure representing a single data entry in the LSM6DSO32 FIFO.
 * 
 * This structure contains the tag and raw data values for a single data entry in the FIFO.
 * The tag represents the type of data stored in the FIFO, and the dataOutRaw array stores
 * the raw data values for the X, Y, and Z axes respectively.
 */
typedef struct {
	uint8_t tag;            /**< Tag representing the type of data */
	int16_t dataOutRaw[3];  /**< Raw data values for X, Y, and Z axes */
} LSM6DSO32_fifo_data_t;

/**
 * @brief Configures the FIFO for the specified sensor.
 * @param sensor The sensor ID.
 * @return esp_err_t Returns ESP_OK if successful, otherwise an error code.
 */
esp_err_t LSM6DSO32_configure_fifo(uint8_t sensor);

/**
 * @brief Reads FIFO data from the specified sensor.
 * @param sensor The sensor ID.
 * @return esp_err_t Returns ESP_OK if successful, otherwise an error code.
 */
esp_err_t LSM6DSO32_readFIFOByID(uint8_t sensor);

/**
 * @brief Parses the gyro data from the FIFO.
 * @param sampleNum The number of samples to parse.
 * @param gyroDataRaw Pointer to an array to store the parsed gyro data.
 * @return esp_err_t Returns ESP_OK if successful, otherwise an error code.
 */
esp_err_t parse_gyro_data(const uint8_t sampleNum, int32_t *gyroDataRaw);

/**
 * @brief Parses the accelerometer data from the FIFO.
 * @param sampleNum The number of samples to parse.
 * @param accDataRaw Pointer to an array to store the parsed accelerometer data.
 * @return esp_err_t Returns ESP_OK if successful, otherwise an error code.
 */
esp_err_t parse_acc_data(const uint8_t sampleNum, int32_t *accDataRaw);

/**
 * @brief Calculates the accelerometer data from raw data.
 * @param sensor The sensor ID.
 * @param rawData Pointer to the raw accelerometer data.
 * @return esp_err_t Returns ESP_OK if successful, otherwise an error code.
 */
esp_err_t calc_acc(uint8_t sensor, int32_t *rawData);

/**
 * @brief Calculates the gyro data from raw data.
 * @param sensor The sensor ID.
 * @param rawData Pointer to the raw gyro data.
 * @return esp_err_t Returns ESP_OK if successful, otherwise an error code.
 */
esp_err_t calc_gyro(uint8_t sensor, int32_t *rawData);

/**
 * @brief Collects gyro data from the sensor.
 * @param gyroDataRaw Pointer to an array to store the collected gyro data.
 * @param sampleValue Pointer to an array to store the sample values.
 * @return esp_err_t Returns ESP_OK if successful, otherwise an error code.
 */
esp_err_t collect_gyro_data(int32_t *gyroDataRaw, int16_t *sampleValue);

/**
 * @brief Collects accelerometer data from the sensor.
 * @param accDataRaw Pointer to an array to store the collected accelerometer data.
 * @param sampleValue Pointer to an array to store the sample values.
 * @return esp_err_t Returns ESP_OK if successful, otherwise an error code.
 */
esp_err_t collect_acc_data(int32_t *accDataRaw, int16_t *sampleValue);

#endif // LSM6DSO32_FIFO_H