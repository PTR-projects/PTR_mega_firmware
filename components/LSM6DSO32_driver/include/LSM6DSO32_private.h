#include "LSM6DSO32_driver.h"



static esp_err_t LSM6DSO32_Write(uint8_t sensor, LSM6DSO32_register_addr_t reg, uint8_t val);
static esp_err_t LSM6DSO32_Read(uint8_t sensor, LSM6DSO32_register_addr_t reg, uint8_t *rx, uint8_t length);
static esp_err_t LSM6DSO32_SetRegister(uint8_t sensor, LSM6DSO32_register_addr_t, uint8_t val);

/**
 * @brief Array of accelerometer sensitivity bits for LSM6DSO32.
 */
const uint8_t LSM6DSAccSensBits[LSM6DS_ACC_FS_LIST_SIZE]=
{
	LSM6DS_CTRL1_XL_ACC_FS_4G,	//LSM6DS_ACC_FS_4G
	LSM6DS_CTRL1_XL_ACC_FS_8G,	//LSM6DS_ACC_FS_8G
	LSM6DS_CTRL1_XL_ACC_FS_16G,	//LSM6DS_ACC_FS_16G
	LSM6DS_CTRL1_XL_ACC_FS_32G 	//LSM6DS_ACC_FS_32G
};

/**
 * @brief Array of accelerometer sensitivity values in mg per LSB for LSM6DSO32.
 */
const float LSM6DSAccSensGPerLsb[LSM6DS_ACC_FS_LIST_SIZE]=
{
	0.000122f,	//LSM6DS_ACC_FS_4G
	0.000244f,	//LSM6DS_ACC_FS_8G
	0.000488f,	//LSM6DS_ACC_FS_16G
	0.000976f 	//LSM6DS_ACC_FS_32G
};

/**
 * @brief Array of gyroscope sensitivity bits for LSM6DSO32.
 */
const uint8_t LSM6DSGyroDpsBits[LSM6DS_GYRO_DPS_LIST_SIZE]=
{
	LSM6DS_CTRL2_G_GYRO_FS_125_DPS , // LSM6DS_GYRO_FS_125_DPS
	LSM6DS_CTRL2_G_GYRO_FS_250_DPS , // LSM6DS_GYRO_FS_250_DPS 
	LSM6DS_CTRL2_G_GYRO_FS_500_DPS , // LSM6DS_GYRO_FS_500_DPS
	LSM6DS_CTRL2_G_GYRO_FS_1000_DPS, // LSM6DS_GYRO_FS_1000_DPS
	LSM6DS_CTRL2_G_GYRO_FS_2000_DPS, // LSM6DS_GYRO_FS_2000_DPS
	
};

/**
 * @brief Array of gyroscope sensitivity values in degrees per second per LSB for LSM6DSO32.
 */
const float LSM6DSGyroDpsPerLsb[LSM6DS_GYRO_DPS_LIST_SIZE]=
{
	0.004375f,	// LSM6DS_GYRO_FS_125_DPS
	0.00875f, 	// LSM6DS_GYRO_FS_250_DPS 
	0.01750f, 	// LSM6DS_GYRO_FS_500_DPS
	0.035f, 	// LSM6DS_GYRO_FS_1000_DPS
	0.070f, 	// LSM6DS_GYRO_FS_2000_DPS
};

/**
 * @brief Structure holding configuration information for LSM6DSO32.
 */
typedef struct
{
	spi_dev_handle_t spi_dev_handle_LSM6DSO32;
	float LSM6DSAccSensMgPerLsbCurrent;
	float LSM6DSGyroDpsPerLsb;
} LSM6DS_config_t;


typedef union{
		uint8_t raw[14];
		struct{
			int16_t temp_raw;

			int16_t gyroX_raw;
			int16_t gyroY_raw;
			int16_t gyroZ_raw;

			int16_t accX_raw;
			int16_t accY_raw;
			int16_t accZ_raw;
		};
} LSM6DSO32_raw_data_t;

typedef struct{
			uint8_t tag;
			int16_t dataOutRaw[3];
} LSM6DSO32_fifo_data_t;	



/**
 * @brief Structure holding raw data and measurements for LSM6DSO32.
 */
typedef struct
{
	LSM6DSO32_raw_data_t rawData;
	LSM6DS_meas_t meas;
	LSM6DS_config_t config;
	float accXoffset;
	float accYoffset;
	float accZoffset;
	float gyroXoffset;
	float gyroYoffset;
	float gyroZoffset;
} LSM6DSO32_t;



esp_err_t LSM6DSO32_readTempByID(uint8_t sensor);
esp_err_t LSM6DSO32_readFIFOByID(uint8_t sensor);
esp_err_t parse_gyro_data(const uint8_t sampleNum, int32_t *gyroDataRaw);
esp_err_t parse_acc_data(const uint8_t sampleNum, int32_t *accDataRaw);
esp_err_t calc_acc(uint8_t sensor, int32_t *rawData);
esp_err_t calc_gyro(uint8_t sensor, int32_t *rawData);
esp_err_t collect_gyro_data(int32_t *gyroDataRaw, int16_t *sampleValue);
esp_err_t collect_acc_data(int32_t *accDataRaw, int16_t *sampleValue);
