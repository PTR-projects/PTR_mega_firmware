#pragma once

#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "BOARD.h"
#include "SPI_driver.h"
#include "esp_log.h"

#ifdef SPI_SLAVE_LSM6DSO32_2_PIN
#define LSM6DSO32_COUNT			 2
#else
#define LSM6DSO32_COUNT			 1
#endif



typedef struct{
	float temp;

	float accX;
	float accY;
	float accZ;

	float gyroX;
	float gyroY;
	float gyroZ;
} LSM6DS_meas_t;




const typedef enum LSM6DSO32_register_addr_t{
	LSM6DS_WHOAMI_RESPONSE_ADDR = 0x6C,   ///< Fixed response value
	LSM6DS_FUNC_CFG_ACCESS_ADDR = 0x1,    ///< Enable embedded functions register
	LSM6DS_INT1_CTRL_ADDR = 0x0D,         ///< Interrupt control for INT 1
	LSM6DS_INT2_CTRL_ADDR = 0x0E,         ///< Interrupt control for INT 2
	LSM6DS_WHOAMI_ADDR = 0x0,             ///< Chip ID register
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

} LSM6DSO32_register_addr_t;

const typedef enum LSM6DSO32_register_t{
	LSM6DS_WHOAMI_RESPONSE,  ///< Fixed response value
	LSM6DS_FUNC_CFG_ACCESS,  ///< Enable embedded functions register
	LSM6DS_INT1_CTRL,       ///< Interrupt control for INT 1
	LSM6DS_INT2_CTRL,   ///< Interrupt control for INT 2
	LSM6DS_WHOAMI,             ///< Chip ID register
	LSM6DS_CTRL1_XL,          ///< Main accelerometer config register
	LSM6DS_CTRL2_G,           ///< Main gyro config register
	LSM6DS_CTRL3_C,         ///< Main configuration register
	LSM6DS_CTRL4_C,
	LSM6DS_CTRL5_C,
	LSM6DS_CTRL6_C,
	LSM6DS_CTRL7_G,
	LSM6DS_CTRL8_XL,
	LSM6DS_CTRL9_XL,
	LSM6DS_CTRL10_C,
	LSM6DS_WAKEUP_SRC,
	LSM6DS_STATUS_REG,
	LSM6DS_OUT_TEMP_L,
	LSM6DS_OUTX_L_G,
	LSM6DS_OUTX_L_A,
	LSM6DS_STEPCOUNTER,
	LSM6DS_TAP_CFG,

	LSM6DS_NUMBER_OF_REGISTERS
} LSM6DSO32_register_t;


const typedef enum LSM6DS_acc_sens_setting_t{
	LSM6DS_ACC_FS_4G,	
	LSM6DS_ACC_FS_8G,	
	LSM6DS_ACC_FS_16G,			
	LSM6DS_ACC_FS_32G,	
	
	LSM6DS_ACC_FS_LIST_SIZE
}LSM6DS_acc_sens_setting_t;


const typedef enum LSM6DS_gyro_dps_setting_t{
	LSM6DS_GYRO_FS_125_DPS,
	LSM6DS_GYRO_FS_250_DPS,
	LSM6DS_GYRO_FS_500_DPS,
	LSM6DS_GYRO_FS_1000_DPS,
	LSM6DS_GYRO_FS_2000_DPS,

	LSM6DS_GYRO_DPS_LIST_SIZE
}LSM6DS_gyro_dps_setting_t;


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

esp_err_t LSM6DSO32_init();
uint8_t LSM6DSO32_WhoAmI(uint8_t sensor);
esp_err_t LSM6DSO32_readMeasByID(uint8_t sensor);
esp_err_t LSM6DSO32_getMeasByID(uint8_t sensor, LSM6DS_meas_t * meas);
esp_err_t LSM6DSO32_readMeasAll();
esp_err_t LSM6DSO32_getMeasAll(LSM6DS_meas_t * meas);
esp_err_t LSM6DSO32_SetAccSens(uint8_t sensor, LSM6DS_acc_sens_setting_t setting);
esp_err_t LSM6DSO32_SetGyroDps(uint8_t sensor, LSM6DS_gyro_dps_setting_t setting);