#include "LSM6DSO32_driver.h"

/*
const LSM6DSO32_register_addr_t LSM6DSO32_register_addr[LSM6DS_NUMBER_OF_REGISTERS]={
	LSM6DS_WHOAMI_RESPONSE_ADDR,
	LSM6DS_FUNC_CFG_ACCESS_ADDR,
	LSM6DS_INT1_CTRL_ADDR,
	LSM6DS_INT2_CTRL_ADDR,
	LSM6DS_WHOAMI_ADDR,
	LSM6DS_CTRL1_XL_ADDR,
	LSM6DS_CTRL2_G_ADDR,
	LSM6DS_CTRL3_C_ADDR,
	LSM6DS_CTRL4_C_ADDR,
	LSM6DS_CTRL5_C_ADDR,
	LSM6DS_CTRL6_C_ADDR,
	LSM6DS_CTRL7_G_ADDR,
	LSM6DS_CTRL8_XL_ADDR,
	LSM6DS_CTRL9_XL_ADDR,
	LSM6DS_CTRL10_C_ADDR,
	LSM6DS_WAKEUP_SRC_ADDR,
	LSM6DS_STATUS_REG_ADDR,
	LSM6DS_OUT_TEMP_L_ADDR,
	LSM6DS_OUTX_L_G_ADDR,
	LSM6DS_OUTX_L_A_ADDR,
	LSM6DS_STEPCOUNTER_ADDR,
	LSM6DS_TAP_CFG_ADDR
};

*/
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