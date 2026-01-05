#ifndef LSM6DSO32_PRIVATE_H
#define LSM6DSO32_PRIVATE_H

#include "LSM6DSO32_driver.h"

/**
 * @brief Configuration structure for LSM6DS sensor.
 */
typedef struct
{
	spi_dev_handle_t spi_dev_handle_LSM6DSO32; /**< SPI device handle for LSM6DSO32 sensor. */
	float LSM6DSAccSensMgPerLsbCurrent; /**< Current accelerometer sensitivity in milli-g per LSB. */
	float LSM6DSGyroDpsPerLsb; /**< Gyroscope sensitivity in degrees per second per LSB. */
} LSM6DS_config_t;

/**
 * @brief Union representing the raw data from LSM6DSO32 sensor.
 */
typedef union{
	uint8_t raw[14]; /**< Raw data array. */
	struct{
		int16_t temp_raw; /**< Raw temperature data. */

		int16_t gyroX_raw; /**< Raw X-axis gyroscope data. */
		int16_t gyroY_raw; /**< Raw Y-axis gyroscope data. */
		int16_t gyroZ_raw; /**< Raw Z-axis gyroscope data. */

		int16_t accX_raw; /**< Raw X-axis accelerometer data. */
		int16_t accY_raw; /**< Raw Y-axis accelerometer data. */
		int16_t accZ_raw; /**< Raw Z-axis accelerometer data. */
	};
} LSM6DSO32_raw_data_t;

/**
 * @brief Structure holding raw data and measurements for LSM6DSO32.
 */
/**
 * @brief Structure representing the LSM6DSO32 sensor data and configuration.
 */
typedef struct
{
	LSM6DSO32_raw_data_t rawData;   /**< Raw sensor data */
	LSM6DS_meas_t meas;             /**< Sensor measurement settings */
	LSM6DS_config_t config;         /**< Sensor configuration settings */
	float accXoffset;               /**< Accelerometer X-axis offset */
	float accYoffset;               /**< Accelerometer Y-axis offset */
	float accZoffset;               /**< Accelerometer Z-axis offset */
	float gyroXoffset;              /**< Gyroscope X-axis offset */
	float gyroYoffset;              /**< Gyroscope Y-axis offset */
	float gyroZoffset;              /**< Gyroscope Z-axis offset */
} LSM6DSO32_t;

/**
 * @brief Read temperature from LSM6DSO32 sensor by ID.
 *
 * @param sensor The ID of the sensor to read temperature from.
 * @return esp_err_t Returns ESP_OK on success, or an error code if reading temperature fails.
 */
esp_err_t LSM6DSO32_readTempByID(uint8_t sensor);

/**
 * @brief Array of LSM6DSO32_t structures representing multiple instances of LSM6DSO32.
 * 
 * This array is used to store multiple instances of the LSM6DSO32 sensor.
 * Each instance is represented by an LSM6DSO32_t structure.
 * The size of the array is determined by the LSM6DSO32_COUNT constant.
 */
static LSM6DSO32_t LSM6DSO32_d[LSM6DSO32_COUNT];

/**
 * @brief Writes a value to the specified register of the LSM6DSO32 sensor.
 *
 * This function writes a single byte value to the specified register of the LSM6DSO32 sensor.
 *
 * @param sensor The sensor number.
 * @param reg The register address to write to.
 * @param val The value to write.
 * @return esp_err_t Returns ESP_OK if the write operation is successful, otherwise returns an error code.
 */
esp_err_t LSM6DSO32_Write(uint8_t sensor, LSM6DSO32_register_addr_t reg, uint8_t val);

/**
 * @brief Reads data from the specified register of the LSM6DSO32 sensor.
 *
 * This function reads a specified number of bytes from the specified register of the LSM6DSO32 sensor.
 *
 * @param sensor The sensor number.
 * @param reg The register address to read from.
 * @param rx Pointer to the buffer where the read data will be stored.
 * @param length The number of bytes to read.
 * @return esp_err_t Returns ESP_OK if the read operation is successful, otherwise returns an error code.
 */
esp_err_t LSM6DSO32_Read(uint8_t sensor, LSM6DSO32_register_addr_t reg, uint8_t *rx, uint8_t length);

/**
 * @brief Sets the value of the specified register of the LSM6DSO32 sensor.
 *
 * This function sets the value of the specified register of the LSM6DSO32 sensor.
 *
 * @param sensor The sensor number.
 * @param reg The register address to set.
 * @param val The value to set.
 * @return esp_err_t Returns ESP_OK if the set operation is successful, otherwise returns an error code.
 */
esp_err_t LSM6DSO32_SetRegister(uint8_t sensor, LSM6DSO32_register_addr_t reg, uint8_t val);

#endif // LSM6DSO32_PRIVATE_H
