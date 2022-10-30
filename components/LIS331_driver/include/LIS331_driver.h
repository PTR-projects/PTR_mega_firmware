#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <driver/spi_master.h>

typedef enum LIS331_type
{
	LIS331_IC_2G 		= 2,
	LIS331_IC_4G 		= 4,
	LIS331_IC_8G 		= 8,
	LIS331HH_IC_6G 		= 6,
	LIS331HH_IC_12G 	= 12,
	LIS331HH_IC_24G 	= 24,
	LIS331_IC_100G 	= 100,
	LIS331_IC_200G 	= 200,
	LIS331_IC_400G 	= 400
} LIS331_type_t;

typedef struct{
	float accX;
	float accY;
	float accZ;
} LIS331_meas_t;

typedef struct {
	union{
		uint8_t raw[6];
		struct{

			int16_t accX_raw;	//2 3
			int16_t accY_raw;	//4 5
			int16_t accZ_raw;	//6 7
		};
	};

	LIS331_meas_t meas;

	float accXoffset;
	float accYoffset;
	float accZoffset;

	float sensor_range;
} LIS331_t;



/*TRANSACTION*/
#define CMD_READ		0x03 	// 11
#define CMD_READ_SINGLE 0x02 	// 10
#define CMD_WRITE		0x01	// 01

/* SPI CONFIG */
#define SPI_BUS SPI2_HOST
#define SPI_DEV SPI_SLAVE_LIS331
#define SPI_CS_PIN 36

#define LIS331_WHO_AM_I                 0x0F

#define LIS331_CTRL_REG1                0x20

#define XEN_MASK						(1 << 0)
#define YEN_MASK						(1 << 1)
#define ZEN_MASK						(1 << 2)
#define DATA_RATE_MASK					(3 << 3)
#define POWER_MODE_MASK					(7 << 5)

#define LIS331_CTRL_REG2                0x21

#define HP_CUTOFF_MASK					(3 << 0)
#define HP_EN1_MASK						(1 << 2)
#define HP_EN2_MASK						(1 << 3)
#define FDS_MASK						(1 << 4)
#define HP_MODE_MASK					(3 << 5)
#define BOOT_MASK						(1 << 7)

#define LIS331_CTRL_REG3                0x22

#define I1_CFG_MASK						(3 << 0)
#define LIR1_MASK						(1 << 2)
#define I2_CFG_MASK						(3 << 3)
#define LIR2_MASK						(1 << 5)
#define PP_OD_MASK						(1 << 6)
#define IHL_MASK						(1 << 7)

#define LIS331_CTRL_REG4                0x23

#define SIM_MASK						(1 << 0)
#define FS_MASK							(3 << 4)
#define BLE_MASK						(1 << 6)
#define BDU_MASK						(1 << 7)

#define LIS331_CTRL_REG5                0x24

#define TURN_ON_MASK					(3 << 0)

#define LIS331_HP_FILTER_RESET          0x25
#define LIS331_REFERENCE                0x26

//STATUS REGISTER

#define LIS331_STATUS_REG               0x27

#define XDA_MASK						(1 << 0)
#define YDA_MASK						(1 << 1)
#define ZDA_MASK						(1 << 2)
#define ZYXDA_MASK						(1 << 3)
#define XOR_MASK						(1 << 4)
#define YOR_MASK						(1 << 5)
#define ZOR_MASK						(1 << 6)
#define ZYXOR_MASK						(1 << 7)

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

esp_err_t 	LIS331_init(LIS331_type_t type); 	//Base init
uint8_t 	LIS331_WhoAmI(void); 				//read device ID default respond 32

esp_err_t LIS331_readMeas(void);
esp_err_t LIS331_getMeas(LIS331_meas_t * meas);
esp_err_t LIS331_getMeasurementXYZ(float* X, float* Y, float* Z); //Get and calculate readings

esp_err_t		 	LIS331_x_axis_set(bool val); 		//Enable/Disable X Axis measurments
bool 				LIS331_x_axis_get(void);				//Check if X Axis measurments are ON
esp_err_t 			LIS331_y_axis_set(bool val); 		//Enable/Disable Y Axis measurments
bool 				LIS331_y_axis_get(void);				//Check if Y Axis measurments are ON
esp_err_t 			LIS331_z_axis_set(bool val);		//Enable/Disable Z Axis measurments
bool 				LIS331_z_axis_get(void);				//Check if Z Axis measurments are ON

esp_err_t 			LIS331_power_mode_set(LIS331_power_mode_t val);
LIS331_power_mode_t LIS331_power_mode_get(void);
esp_err_t 			LIS331_data_rate_set(LIS331_data_rate_t val);
LIS331_data_rate_t 	LIS331_data_rate_get(void);
esp_err_t 			LIS331_boot(void);
esp_err_t 			LIS331_hp_filter_set(LIS331_hp_mode_t val);
LIS331_hp_mode_t 	LIS331_hp_filter_get(void);
esp_err_t 			LIS331_hp_en_set(uint8_t interup, bool val);
bool 				LIS331_hp_en_get(uint8_t interup);
esp_err_t 			LIS331_hp_cutoff_set(LIS331_hp_cutoff_t val);
LIS331_hp_cutoff_t 	LIS331_hp_cutoff_get(void);
esp_err_t 			LIS331_bdu_set(bool val);
bool 				LIS331_bdu_get(void);
esp_err_t 			LIS331_ble_set(bool val);
bool 				LIS331_ble_get(void);
esp_err_t 			LIS331_range_set(LIS331_range_t val);
LIS331_range_t 		LIS331_range_get(void);
