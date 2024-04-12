#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include "SPI_driver.h"

/**
 * @brief Enumeration of the different ranges of xLIS331x accelerometers that are supported.
 */
typedef enum{
	LIS331_RANGE_LOW  = 0,
	LIS331_RANGE_MID  = 1,
	LIS331_RANGE_HIGH = 3
} LIS331_range_e;

/**
 * @brief Structure to hold the measured acceleration data for the LIS331 accelerometer.
 */
typedef struct{
	float accX;		/*!< Measured acceleration along the X-axis. */
	float accY;		/*!< Measured acceleration along the Y-axis. */
	float accZ;		/*!< Measured acceleration along the Z-axis. */
} LIS331_meas_t;


/**
 * @brief Structure to hold the raw data and calculated acceleration data for the LIS331 accelerometer.
 */
typedef struct {
	spi_dev_handle_t spi_handle;

	union{								/*!< Union containing the raw data from the accelerometer as an array and as individual fields. */
		uint8_t raw[6];					/*!< Array holding the raw data from the accelerometer. */
		struct{							/*!< Struct containing the raw data from the accelerometer as individual fields. */
			int16_t accX_raw;			/*!< Raw data for the acceleration along the X-axis. */
			int16_t accY_raw;			/*!< Raw data for the acceleration along the Y-axis. */
			int16_t accZ_raw;			/*!< Raw data for the acceleration along the Z-axis. */
		};
	};

	LIS331_meas_t meas;					/*!< Structure containing the measured acceleration data. */

	float accXoffset;					/*!< Offset for the acceleration data along the X-axis. */
	float accYoffset;					/*!< Offset for the acceleration data along the Y-axis. */
	float accZoffset;					/*!< Offset for the acceleration data along the Z-axis. */

	int sensor_range;					/*!< The range of the accelerometer. */
} LIS331_t;



/*TRANSACTION*/
#define LIS331_CMD_READ		0x03 	// 11
#define LIS331_CMD_READ_SINGLE 0x02 	// 10
#define LIS331_CMD_WRITE		0x01	// 01

/* SPI CONFIG */
#define LIS331_WHO_AM_I                 0x0F

#define LIS331_CTRL_REG1                0x20

#define LIS331_XEN_MASK					(1 << 0)
#define LIS331_YEN_MASK					(1 << 1)
#define LIS331_ZEN_MASK					(1 << 2)
#define LIS331_DATA_RATE_MASK			(3 << 3)
#define LIS331_POWER_MODE_MASK			(7 << 5)

#define LIS331_CTRL_REG2                0x21

#define LIS331_HP_CUTOFF_MASK			(3 << 0)
#define LIS331_HP_EN1_MASK				(1 << 2)
#define LIS331_HP_EN2_MASK				(1 << 3)
#define LIS331_FDS_MASK					(1 << 4)
#define LIS331_HP_MODE_MASK				(3 << 5)
#define LIS331_BOOT_MASK				(1 << 7)

#define LIS331_CTRL_REG3                0x22

#define LIS331_I1_CFG_MASK				(3 << 0)
#define LIS331_LIR1_MASK				(1 << 2)
#define LIS331_I2_CFG_MASK				(3 << 3)
#define LIS331_LIR2_MASK				(1 << 5)
#define LIS331_PP_OD_MASK				(1 << 6)
#define LIS331_IHL_MASK					(1 << 7)

#define LIS331_CTRL_REG4                0x23

#define LIS331_SIM_MASK					(1 << 0)
#define LIS331_FS_MASK					(3 << 4)
#define LIS331_BLE_MASK					(1 << 6)
#define LIS331_BDU_MASK					(1 << 7)

#define LIS331_CTRL_REG5                0x24

#define LIS331_TURN_ON_MASK				(3 << 0)

#define LIS331_HP_FILTER_RESET          0x25
#define LIS331_REFERENCE                0x26

//STATUS REGISTER

#define LIS331_STATUS_REG               0x27

#define LIS331_XDA_MASK					(1 << 0)
#define LIS331_YDA_MASK					(1 << 1)
#define LIS331_ZDA_MASK					(1 << 2)
#define LIS331_ZYXDA_MASK				(1 << 3)
#define LIS331_XOR_MASK					(1 << 4)
#define LIS331_YOR_MASK					(1 << 5)
#define LIS331_ZOR_MASK					(1 << 6)
#define LIS331_ZYXOR_MASK				(1 << 7)

// OUTPUT REGISTERS
#define LIS331_OUT_X_L	                0x28
#define LIS331_OUT_X_H	                0x29
#define LIS331_OUT_Y_L	                0x2A
#define LIS331_OUT_Y_H	                0x2B
#define LIS331_OUT_Z_L	                0x2C
#define LIS331_OUT_Z_H	                0x2D


typedef enum LIS331_range
{
	LIS331_RANGE_100G = (0 << 4),
	LIS331_RANGE_200G = (1 << 4),
	LIS331_RANGE_400G = (3 << 4),

}LIS331_range_t;

typedef enum LIS331_power_mode
{
	LIS331_POWER_MODE_DOWN = 	(0 << 5),
	LIS331_POWER_MODE_NORMAL = 	(1 << 5),
	LIS331_POWER_MODE_LOW05 = 	(2 << 5),
	LIS331_POWER_MODE_LOW1 = 	(3 << 5),
	LIS331_POWER_MODE_LOW2 = 	(4 << 5),
	LIS331_POWER_MODE_LOW5 = 	(5 << 5),
	LIS331_POWER_MODE_LOW10 = 	(6 << 5),

}LIS331_power_mode_t;

typedef enum LIS331_data_rate
{
	LIS331_DATA_RATE_50 = 	(0 << 3),
	LIS331_DATA_RATE_100 = 	(1 << 3),
	LIS331_DATA_RATE_400 = 	(2 << 3),
	LIS331_DATA_RATE_1000 = (3 << 3),

}LIS331_data_rate_t;


typedef enum LIS331_hp_mode
{
	LIS331_HP_MODE_NORMAL = 	(0 << 5),
	LIS331_HP_MODE_REF = 		(1 << 5),
	LIS331_HP_MODE_NORMAL_2 = 	(2 << 5), //Check Datasheet

}LIS331_hp_mode_t;

typedef enum LIS331_FDS
{
	LIS331_FDS_BYPASS = 	(0 << 4),
	LIS331_FDS_OUTPUT = 	(1 << 4),


}LIS331_FDS_t;

typedef enum LIS331_hp_cutoff
{
	LIS331_HP_CUTOFF_8 = (0 << 0),
	LIS331_HP_CUTOFF_16 = (1 << 0),
	LIS331_HP_CUTOFF_32 = (2 << 0),
	LIS331_HP_CUTOFF_64 = (3 << 0),

}LIS331_hp_cutoff_t;

/**
* @brief Initializes the LIS331 sensor with the specified range (low, mid, high).
* @param[in] type The type of xLIS331x sensor.
* @return ESP_OK if initialization was successful, ESP_FAIL otherwise.
*/
esp_err_t 	LIS331_init(LIS331_range_e range); 	//Base init

/**
* @brief Reads the device ID of the LIS331 sensor.
* @return The device ID of the LIS331 sensor. The correct response is 0x0F.
*/
uint8_t 	LIS331_WhoAmI(void); 				//read device ID default respond 32

/**
* @brief Reads the raw measurement data from the LIS331 sensor.
* @return ESP_OK if the measurement was successful, ESP_FAIL otherwise.
*/
esp_err_t LIS331_readMeas();

/**
* @brief Gets the processed measurement data from the LIS331 sensor.
* @param[out] meas A pointer to the measurement data structure.
* @return ESP_OK if the measurement was successful, ESP_FAIL otherwise.
*/
esp_err_t LIS331_getMeas(uint8_t sensor, LIS331_meas_t * meas);

/**
* @brief Gets and calculates the X, Y, and Z readings from the LIS331 sensor.
* @param[out] X The X reading.
* @param[out] Y The Y reading.
* @param[out] Z The Z reading.
* @return ESP_OK if the measurement was successful, ESP_FAIL otherwise.
*/
esp_err_t LIS331_getMeasurementXYZ(uint8_t sensor, float* X, float* Y, float* Z); //Get and calculate readings

/**
* @brief Enables or disables measurement on the X axis of the LIS331 sensor.
* @param[in] val True to enable measurement, false to disable.
* @return ESP_OK if the operation was successful, ESP_FAIL otherwise.
*/
esp_err_t		 	LIS331_x_axis_set(uint8_t sensor, bool val);

/**
* @brief Gets the status of measurement on the X axis of the LIS331 sensor.
* @return True if measurement is enabled, false if disabled.
*/
bool 				LIS331_x_axis_get(uint8_t sensor);

/**
* @brief Enables or disables measurement on the Y axis of the LIS331 sensor.
* @param[in] val True to enable measurement, false to disable.
* @return ESP_OK if the operation was successful, ESP_FAIL otherwise.
*/
esp_err_t 			LIS331_y_axis_set(uint8_t sensor, bool val); 		//Enable/Disable Y Axis measurments

/**
* @brief Gets the status of measurement on the Y axis of the LIS331 sensor.
* @return True if measurement is enabled, false if disabled.
*/
bool 				LIS331_y_axis_get(uint8_t sensor);					//Check if Y Axis measurments are ON

/**
* @brief Enables or disables measurement on the Z axis of the LIS331 sensor.
* @param[in] val True to enable measurement, false to disable.
* @return ESP_OK if the operation was successful, ESP_FAIL otherwise.
*/
esp_err_t 			LIS331_z_axis_set(uint8_t sensor, bool val);		//Enable/Disable Z Axis measurments

/**
* @brief Gets the status of measurement on the Z axis of the LIS331 sensor.
* @return True if measurement is enabled, false if disabled.
*/
bool 				LIS331_z_axis_get(uint8_t sensor);					//Check if Z Axis measurments are ON

/**
* @brief Sets the power mode of the LIS331 sensor.
* @param[in] val The power mode to set.
* @return ESP_OK if the operation was successful, ESP_FAIL otherwise.
*/
esp_err_t 			LIS331_power_mode_set(uint8_t sensor, LIS331_power_mode_t val);

/**
* @brief Gets the current power mode of the LIS331 sensor.
* @return The current power mode.
*/
LIS331_power_mode_t LIS331_power_mode_get(uint8_t sensor);

/**
* @brief Sets the data rate of the LIS331 sensor.
* @param[in] val The data rate to set.
* @return ESP_OK if the operation was successful, ESP_FAIL otherwise.
*/
esp_err_t 			LIS331_data_rate_set(uint8_t sensor, LIS331_data_rate_t val);

/**
* @brief Gets the current data rate of the LIS331 sensor.
* @return The current data rate.
*/
LIS331_data_rate_t 	LIS331_data_rate_get(uint8_t sensor);

/**
* @brief Boots the LIS331 sensor.
* @return ESP_OK if the operation was successful, ESP_FAIL otherwise.
*/
esp_err_t 			LIS331_boot(void);

/**
* @brief Sets the high-pass filter mode of the LIS331 sensor.
* @param[in] val The high-pass filter mode to set.
* @return ESP_OK if the operation was successful, ESP_FAIL otherwise.
*/
esp_err_t 			LIS331_hp_filter_set(LIS331_hp_mode_t val);

/**
* @brief Gets the current high-pass filter mode of the LIS331 sensor.
* @return The current high-pass filter mode.
*/
LIS331_hp_mode_t 	LIS331_hp_filter_get(void);

/**
* @brief Enables or disables the high-pass filter for a specific interrupt on the LIS331 sensor.
* @param[in] interup The interrupt to enable or disable the high-pass filter for.
* @param[in] val True to enable the high-pass filter, false to disable.
* @return ESP_OK if the operation was successful, ESP_FAIL otherwise.
*/
esp_err_t 			LIS331_hp_en_set(uint8_t interup, bool val);

/**
* @brief Gets the status of the high-pass filter for a specific interrupt on the LIS331 sensor.
* @param[in] interup The interrupt to check the status of the high-pass filter for.
* @return True if the high-pass filter is enabled, false if disabled.
*/
bool 				LIS331_hp_en_get(uint8_t interup);

/**
* @brief Sets the high pass filter cutoff frequency of the LIS331 sensor.
* @param[in] val The cutoff frequency to set.
* @return ESP_OK if the operation was successful, ESP_FAIL otherwise.
*/
esp_err_t 			LIS331_hp_cutoff_set(LIS331_hp_cutoff_t val);

/**
* @brief Gets the current high-pass filter cutoff frequency of the LIS331 sensor.
* @return The current high-pass filter cutoff frequency.
*/
LIS331_hp_cutoff_t 	LIS331_hp_cutoff_get(void);

/**
* @brief Enables or disables the block data update mode of the LIS331 sensor.
* @param[in] val True to enable block data update mode, false to disable.
* @return ESP_OK if the operation was successful, ESP_FAIL otherwise.
*/
esp_err_t 			LIS331_bdu_set(bool val);

/**
* @brief Gets the status of the block data update mode of the LIS331 sensor.
* @return True if block data update mode is enabled, false if disabled.
*/
bool 				LIS331_bdu_get(void);

/**
* @brief Enables or disables the Bluetooth Low Energy (BLE) feature of the LIS331 sensor.
* @param[in] val True to enable BLE, false to disable.
* @return ESP_OK if the operation was successful, ESP_FAIL otherwise.
*/
esp_err_t 			LIS331_ble_set(bool val);

/**
* @brief Gets the status of the Bluetooth Low Energy (BLE) feature of the LIS331 sensor.
* @return True if BLE is enabled, false if disabled.
*/
bool 				LIS331_ble_get(void);

/**
* @brief Sets the measurement range of the LIS331 sensor.
* @param[in] val The range to set.
* @return ESP_OK if the operation was successful, ESP_FAIL otherwise.
*/
esp_err_t 			LIS331_range_set(LIS331_range_t val);

/**
* @brief Gets the current measurement range of the LIS331 sensor.
* @return The current measurement range.
*/
LIS331_range_t 		LIS331_range_get(void);