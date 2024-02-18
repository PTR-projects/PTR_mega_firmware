#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include "SPI_driver.h"

typedef enum{
	LIS331_RANGE_LOW  = 0,
	LIS331_RANGE_MID  = 1,
	LIS331_RANGE_HIGH = 3
} LIS331_range_e;

typedef struct{
	float accX;
	float accY;
	float accZ;
} LIS331_meas_t;

typedef struct {
	spi_dev_handle_t spi_handle;

	union{
		uint8_t raw[6];
		struct{
			int16_t accX_raw;	//1 2
			int16_t accY_raw;	//3 4
			int16_t accZ_raw;	//5 6
		};
	};

	LIS331_meas_t meas;

	float accXoffset;
	float accYoffset;
	float accZoffset;

	int sensor_range;
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

esp_err_t 	LIS331_init(LIS331_range_e range); 	//Base init

esp_err_t LIS331_readMeas();
esp_err_t LIS331_getMeas(uint8_t sensor, LIS331_meas_t * meas);
esp_err_t LIS331_getMeasurementXYZ(uint8_t sensor, float* X, float* Y, float* Z); //Get and calculate readings

esp_err_t		 	LIS331_x_axis_set(uint8_t sensor, bool val); 		//Enable/Disable X Axis measurments
bool 				LIS331_x_axis_get(uint8_t sensor);					//Check if X Axis measurments are ON
esp_err_t 			LIS331_y_axis_set(uint8_t sensor, bool val); 		//Enable/Disable Y Axis measurments
bool 				LIS331_y_axis_get(uint8_t sensor);					//Check if Y Axis measurments are ON
esp_err_t 			LIS331_z_axis_set(uint8_t sensor, bool val);		//Enable/Disable Z Axis measurments
bool 				LIS331_z_axis_get(uint8_t sensor);					//Check if Z Axis measurments are ON

esp_err_t 			LIS331_power_mode_set(uint8_t sensor, LIS331_power_mode_t val);
LIS331_power_mode_t LIS331_power_mode_get(uint8_t sensor);
esp_err_t 			LIS331_data_rate_set(uint8_t sensor, LIS331_data_rate_t val);
LIS331_data_rate_t 	LIS331_data_rate_get(uint8_t sensor);
esp_err_t 			LIS331_boot(uint8_t sensor);
esp_err_t 			LIS331_hp_filter_set(uint8_t sensor, LIS331_hp_mode_t val);
LIS331_hp_mode_t 	LIS331_hp_filter_get(uint8_t sensor);
esp_err_t 			LIS331_hp_en_set(uint8_t sensor, uint8_t interrupt, bool val);
bool 				LIS331_hp_en_get(uint8_t sensor, uint8_t interrupt);
esp_err_t 			LIS331_hp_cutoff_set(uint8_t sensor, LIS331_hp_cutoff_t val);
LIS331_hp_cutoff_t 	LIS331_hp_cutoff_get(uint8_t sensor);
esp_err_t 			LIS331_bdu_set(uint8_t sensor, bool val);
bool 				LIS331_bdu_get(uint8_t sensor);
esp_err_t 			LIS331_ble_set(uint8_t sensor, bool val);
bool 				LIS331_ble_get(uint8_t sensor);
esp_err_t 			LIS331_range_set(uint8_t sensor, LIS331_range_t val);
LIS331_range_t 		LIS331_range_get(uint8_t sensor);
