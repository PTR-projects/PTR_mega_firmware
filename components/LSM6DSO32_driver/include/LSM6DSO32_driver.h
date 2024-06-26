#ifndef LSM6DSO32_DRIVER_H
#define LSM6DSO32_DRIVER_H


#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "BOARD.h"
#include "SPI_driver.h"
#include "esp_log.h"

/**
 * @brief IMU measurement data
 */
typedef struct{
	float temp;			/*!< Temperature */

	float accX;			/*!< X axis acceleration */
	float accY;			/*!< Y axis acceleration */
	float accZ;			/*!< Z axis acceleration */

	float gyroX;		/*!< X axis angular velocity */
	float gyroY;		/*!< Y axis angular velocity */
	float gyroZ;		/*!< Z axis angular velocity */
} LSM6DS_meas_t;

const typedef enum LSM6DSO32_register_addr_t{
	LSM6DS_WHOAMI_RESPONSE = 0x6C,   ///< Fixed response value
	LSM6DS_FUNC_CFG_ACCESS_ADDR = 0x1,    ///< Enable embedded functions register
	LSM6DS_INT1_CTRL_ADDR = 0x0D,         ///< Interrupt control for INT 1
	LSM6DS_INT2_CTRL_ADDR = 0x0E,         ///< Interrupt control for INT 2
	LSM6DS_WHOAMI_ADDR = 0x0F,             ///< Chip ID register
	LSM6DS_CTRL1_XL_ADDR = 0x10,          ///< Main accelerometer config register
	LSM6DS_CTRL2_G_ADDR = 0x11,           ///< Main gyro config register
	LSM6DS_CTRL3_C_ADDR = 0x12,           ///< Main configuration register
	LSM6DS_CTRL4_C_ADDR = 0x13,
	LSM6DS_CTRL5_C_ADDR = 0x14,
	LSM6DS_CTRL6_C_ADDR = 0x15,
	LSM6DS_CTRL7_G_ADDR = 0x16,
	LSM6DS_CTRL8_XL_ADDR = 0x17,          ///< High and low pass for accel
	LSM6DS_CTRL9_XL_ADDR = 0x18,          ///< Data Enable Configuration
	LSM6DS_CTRL10_C_ADDR = 0x19,          ///< Main configuration register
	LSM6DS_WAKEUP_SRC_ADDR = 0x1B,        ///< Why we woke up
	LSM6DS_STATUS_REG_ADDR = 0X1E,        ///< Status register
	LSM6DS_OUT_TEMP_L_ADDR = 0x20,        ///< First data register (temperature low)
	LSM6DS_OUTX_L_G_ADDR = 0x22,          ///< First gyro data register
	LSM6DS_OUTX_L_A_ADDR = 0x28,          ///< First accel data register
	LSM6DS_STEPCOUNTER_ADDR = 0x4B,       ///< 16-bit step counter
	LSM6DS_TAP_CFG_ADDR = 0x58,           ///< Tap/pedometer configuration
	LSM6DS_FIFO_CTRL1_ADDR = 0x07,
	LSM6DS_FIFO_CTRL2_ADDR = 0x08,
	LSM6DS_FIFO_CTRL3_ADDR = 0x09,
	LSM6DS_FIFO_CTRL4_ADDR = 0x0A,
	LSM6DS_FIFO_STATUS1_ADDR = 0x3A,
	LSM6DS_FIFO_STATUS2_ADDR = 0x3B,
	LSM6DS_FIFO_DATA_OUT_TAG_ADDR = 0x78,
	LSM6DS_FIFO_DATA_OUT_X_L = 0x79,
	

} LSM6DSO32_register_addr_t;


const typedef enum {
	LSM6DS_ACC_FS_4G,	
	LSM6DS_ACC_FS_8G,	
	LSM6DS_ACC_FS_16G,			
	LSM6DS_ACC_FS_32G,	
	
	LSM6DS_ACC_FS_LIST_SIZE
}LSM6DS_acc_sens_setting_t;


const typedef enum {
	LSM6DS_GYRO_FS_125_DPS,
	LSM6DS_GYRO_FS_250_DPS,
	LSM6DS_GYRO_FS_500_DPS,
	LSM6DS_GYRO_FS_1000_DPS,
	LSM6DS_GYRO_FS_2000_DPS,

	LSM6DS_GYRO_DPS_LIST_SIZE
}LSM6DS_gyro_dps_setting_t;





// CTRL1_XL
#define LSM6DS_CTRL1_XL_ACC_RATE_SHUTDOWN	(0  << 4)
#define LSM6DS_CTRL1_XL_ACC_RATE_1_6_HZ		(11 << 4)
#define LSM6DS_CTRL1_XL_ACC_RATE_12_5_HZ	(1  << 4)
#define LSM6DS_CTRL1_XL_ACC_RATE_26_HZ		(2  << 4)
#define LSM6DS_CTRL1_XL_ACC_RATE_52_HZ		(3  << 4)
#define LSM6DS_CTRL1_XL_ACC_RATE_104_HZ		(4  << 4)
#define LSM6DS_CTRL1_XL_ACC_RATE_208_HZ		(5  << 4)
#define LSM6DS_CTRL1_XL_ACC_RATE_416_HZ		(6  << 4)
#define LSM6DS_CTRL1_XL_ACC_RATE_833_HZ		(7  << 4)
#define LSM6DS_CTRL1_XL_ACC_RATE_1_66K_HZ	(8  << 4)
#define LSM6DS_CTRL1_XL_ACC_RATE_3_33K_HZ	(9  << 4)
#define LSM6DS_CTRL1_XL_ACC_RATE_6_66K_HZ	(10 << 4)

#define LSM6DS_CTRL1_XL_ACC_FS_4G			(0 << 2)
#define LSM6DS_CTRL1_XL_ACC_FS_8G			(2 << 2)
#define LSM6DS_CTRL1_XL_ACC_FS_16G			(3 << 2)
#define LSM6DS_CTRL1_XL_ACC_FS_32G			(1 << 2)
#define LSM6DS_CTRL1_ACC_LPF2_EN			(1 << 1)

// CTRL2_G
#define LSM6DS_CTRL2_G_GYRO_RATE_SHUTDOWN	(0  << 4)
#define LSM6DS_CTRL2_G_GYRO_RATE_12_5_HZ	(1  << 4)
#define LSM6DS_CTRL2_G_GYRO_RATE_26_HZ		(2  << 4)
#define LSM6DS_CTRL2_G_GYRO_RATE_52_HZ		(3  << 4)
#define LSM6DS_CTRL2_G_GYRO_RATE_104_HZ		(4  << 4)
#define LSM6DS_CTRL2_G_GYRO_RATE_208_HZ		(5  << 4)
#define LSM6DS_CTRL2_G_GYRO_RATE_416_HZ		(6  << 4)
#define LSM6DS_CTRL2_G_GYRO_RATE_833_HZ		(7  << 4)
#define LSM6DS_CTRL2_G_GYRO_RATE_1_66K_HZ	(8  << 4)
#define LSM6DS_CTRL2_G_GYRO_RATE_3_33K_HZ	(9  << 4)
#define LSM6DS_CTRL2_G_GYRO_RATE_6_66K_HZ	(10 << 4)

#define LSM6DS_CTRL2_G_GYRO_FS_125_DPS		(1 << 1)
#define LSM6DS_CTRL2_G_GYRO_FS_250_DPS		(0 << 2)
#define LSM6DS_CTRL2_G_GYRO_FS_500_DPS		(1 << 2)
#define LSM6DS_CTRL2_G_GYRO_FS_1000_DPS		(2 << 2)
#define LSM6DS_CTRL2_G_GYRO_FS_2000_DPS		(3 << 2)

// CTRL3_C
#define LSM6DS_CTRL3_BOOT		(1 << 7)
#define LSM6DS_CTRL3_BDU		(1 << 6)
#define LSM6DS_CTRL3_INT_L		(1 << 5)
#define LSM6DS_CTRL3_INT_H		(0 << 5)
#define LSM6DS_CTRL3_INT_OD		(1 << 4)
#define LSM6DS_CTRL3_INT_PP		(0 << 4)
#define LSM6DS_CTRL3_3WIRE		(1 << 3)
#define LSM6DS_CTRL3_INC		(1 << 2)
#define LSM6DS_CTRL3_RST		(1 << 0)

// CTRL4_C
#define LSM6DS_CTRL4_GYRO_SLEEP		(1 << 6)
#define LSM6DS_CTRL4_INT1_ALL		(1 << 5)
#define LSM6DS_CTRL4_INT12_SEP		(0 << 5)
#define LSM6DS_CTRL4_DRDY_MASK		(1 << 3)
#define LSM6DS_CTRL4_I2C_DIS		(1 << 2)
#define LSM6DS_CTRL4_GYRO_LPF1_EN	(1 << 1)

// CTRL5_C
#define LSM6DS_CTRL5_ACC_ULP_EN			(1 << 7)
#define LSM6DS_CTRL5_ACC_ULP_DIS		(0 << 7)
#define LSM6DS_CTRL5_ROUNDING_DIS		(0 << 5)
#define LSM6DS_CTRL5_ROUNDING_ACC		(1 << 5)
#define LSM6DS_CTRL5_ROUNDING_GYRO		(2 << 5)
#define LSM6DS_CTRL5_ROUNDING_ACC_GYRO	(3 << 5)
#define LSM6DS_CTRL5_GYRO_ST_DIS		(0 << 2)
#define LSM6DS_CTRL5_GYRO_ST_POS		(1 << 2)
#define LSM6DS_CTRL5_GYRO_ST_NEG		(3 << 2)
#define LSM6DS_CTRL5_ACC_ST_DIS			(0 << 0)
#define LSM6DS_CTRL5_ACC_ST_POS			(1 << 0)
#define LSM6DS_CTRL5_ACC_ST_NEG			(3 << 0)

// CTRL6_C											<<-------------- do uzupe�nienia od mniej potrzebne
#define LSM6DS_CTRL6_GYRO_LPF1_0		(0 << 0)
#define LSM6DS_CTRL6_GYRO_LPF1_1		(1 << 0)
#define LSM6DS_CTRL6_GYRO_LPF1_2		(2 << 0)
#define LSM6DS_CTRL6_GYRO_LPF1_3		(3 << 0)
#define LSM6DS_CTRL6_GYRO_LPF1_4		(4 << 0)
#define LSM6DS_CTRL6_GYRO_LPF1_5		(5 << 0)
#define LSM6DS_CTRL6_GYRO_LPF1_6		(6 << 0)
#define LSM6DS_CTRL6_GYRO_LPF1_7		(7 << 0)

// CTRL7_G
#define LSM6DS_CTRL7_HPM_EN			(1 << 7)
#define LSM6DS_CTRL7_HPF_EN			(1 << 6)
#define LSM6DS_CTRL7_HPF_16mHZ		(0 << 4)
#define LSM6DS_CTRL7_HPF_65mHZ		(1 << 4)
#define LSM6DS_CTRL7_HPF_260mHZ		(2 << 4)
#define LSM6DS_CTRL7_HPF_1_04HZ		(3 << 4)
#define LSM6DS_CTRL7_USR_OFF_EN		(1 << 1)

// CTRL8_XL
#define LSM6DS_CTRL8_ACC_LPF			(0 << 2)
#define LSM6DS_CTRL8_ACC_HPF			(1 << 2)
#define LSM6DS_CTRL8_FILTER_ODR_4		(0 << 5)
#define LSM6DS_CTRL8_FILTER_ODR_10		(1 << 5)
#define LSM6DS_CTRL8_FILTER_ODR_20		(2 << 5)
#define LSM6DS_CTRL8_FILTER_ODR_45		(3 << 5)
#define LSM6DS_CTRL8_FILTER_ODR_100		(4 << 5)
#define LSM6DS_CTRL8_FILTER_ODR_200		(5 << 5)
#define LSM6DS_CTRL8_FILTER_ODR_400		(6 << 5)
#define LSM6DS_CTRL8_FILTER_ODR_800		(7 << 5)

// CTRL9_XL


// CTRL10_C
/**
 * @brief Initializes LSM6DSO32 sensors.
 *
 * This function initializes the SPI communication for LSM6DSO32 sensors and configures
 * each sensor with default settings. It sets up the accelerometer and gyroscope parameters,
 * performs a WHO_AM_I check, and initializes internal data structures.
 *
 * @return esp_err_t ESP_OK if successful, otherwise an error code.
 */
esp_err_t LSM6DSO32_init();

/**
 * @brief Retrieves the WHO_AM_I response from the LSM6DSO32 sensor.
 *
 * @param sensor Sensor number.
 * @return uint8_t WHO_AM_I response.
 */
uint8_t LSM6DSO32_WhoAmI(uint8_t sensor);

/**
 * @brief Reads measurement data from a specified LSM6DSO32 sensor.
 *
 * @param sensor Sensor number.
 * @return esp_err_t ESP_OK if successful, otherwise an error code.
 *
 * This function reads measurement data from the LSM6DSO32 sensor specified by the sensor number.
 * It performs the read operation and, if successful, calculates the accelerometer, gyroscope, and temperature measurements.
 */
esp_err_t LSM6DSO32_readMeasByID(uint8_t sensor);

/**
 * @brief Retrieves measurement data from a specified LSM6DSO32 sensor.
 *
 * @param sensor Sensor number.
 * @param meas Pointer to LSM6DS_meas_t structure to store measurement data.
 * @return esp_err_t ESP_OK if successful, otherwise an error code.
 */
esp_err_t LSM6DSO32_getMeas(uint8_t sensor, LSM6DS_meas_t * meas);

/**
 * @brief Reads all measurement data from all LSM6DSO32 sensors.
 *
 * @return esp_err_t ESP_OK if successful, otherwise an error code.
 */
esp_err_t LSM6DSO32_readMeasAll();

/**
 * @brief Retrieves measurement data from all LSM6DSO32 sensors.
 *
 * @param meas Pointer to an array of LSM6DS_meas_t structures to store measurement data.
 * @return esp_err_t ESP_OK if successful, otherwise an error code.
 */
esp_err_t LSM6DSO32_getMeasAll(LSM6DS_meas_t * meas);

/**
 * @brief Sets the accelerometer sensitivity for the LSM6DSO32 sensor.
 *
 * @param sensor Sensor number.
 * @param setting New sensitivity setting.
 * @return esp_err_t ESP_OK if successful, otherwise an error code.
 */

esp_err_t LSM6DSO32_SetAccSens(uint8_t sensor, LSM6DS_acc_sens_setting_t setting);

/**
 * @brief Sets the gyroscope sensitivity for the LSM6DSO32 sensor.
 *
 * @param sensor Sensor number.
 * @param setting New sensitivity setting.
 * @return esp_err_t ESP_OK if successful, otherwise an error code.
 */
esp_err_t LSM6DSO32_SetGyroDps(uint8_t sensor, LSM6DS_gyro_dps_setting_t setting);

/**
 * @brief TODO
 */
esp_err_t LSM6DSO32_calibrateGyroAll(float gain);

esp_err_t LSM6DSO32_readMeasByID(uint8_t sensor);
esp_err_t LSM6DSO32_calibrateGyro(uint8_t sensor, float gain);

#endif // LSM6DSO32_DRIVER_H
