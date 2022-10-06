#pragma once
#include "esp_err.h"
#include <stdbool.h>

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






#define LIS331_WHO_AM_I                  0x0F

#define LIS331_CTRL_REG1                 0x20

#define Xen_mask						(1 << 0)
#define Yen_mask						(1 << 1)
#define Zen_mask						(1 << 2)
#define DataRate_mask					(3 << 3)
#define PowerMode_mask					(7 << 5)

#define LIS331_CTRL_REG2                 0x21

#define HP_Cutoff_mask					(3 << 0)
#define HP_En1_mask						(1 << 2)
#define HP_En2_mask						(1 << 3)
#define FDS_mask						(1 << 4)
#define HP_mode_mask					(3 << 5)
#define Boot_mask						(1 << 7)

#define LIS331_CTRL_REG3                 0x22

#define I1_CFG_mask						(3 << 0)
#define LIR1_mask						(1 << 2)
#define I2_CFG_mask						(3 << 3)
#define LIR2_mask						(1 << 5)
#define PP_OD_mask						(1 << 6)
#define IHL_mask						(1 << 7)

#define LIS331_CTRL_REG4                 0x23

#define SIM_mask						(1 << 0)
#define FS_mask							(3 << 4)
#define BLE_mask						(1 << 6)
#define BDU_mask						(1 << 7)

#define LIS331_CTRL_REG5                 0x24

#define TurnOn_mask						(3 << 0)

#define LIS331_HP_FILTER_RESET           0x25
#define LIS331_REFERENCE                 0x26

//STATUS REGISTER

#define LIS331_STATUS_REG                0x27

#define XDA_mask						(1 << 0)
#define YDA_mask						(1 << 1)
#define ZDA_mask						(1 << 2)
#define ZYXDA_mask						(1 << 3)
#define XOR_mask						(1 << 4)
#define YOR_mask						(1 << 5)
#define ZOR_mask						(1 << 6)
#define ZYXOR_mask						(1 << 7)

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

esp_err_t LIS331_init(LIS331_type_t type); 	//Base init
uint8_t LIS331_WhoAmI(void); 				//read device ID default respond 32
esp_err_t LIS331_xyz_acc_calc(void); 		//Get and calculate readings

esp_err_t LIS331_x_axis_set(bool val); 		//Enable/Disable X Axis measurments
bool LIS331_x_axis_get(void);				//Check if X Axis measurments are ON
esp_err_t LIS331_y_axis_set(bool val); 		//Enable/Disable Y Axis measurments
bool LIS331_y_axis_get(void);				//Check if Y Axis measurments are ON
esp_err_t LIS331_z_axis_set(bool val);		//Enable/Disable Z Axis measurments
bool LIS331_z_axis_get(void);				//Check if Z Axis measurments are ON

esp_err_t LIS331_power_mode_set(LIS331_power_mode_t val);
LIS331_power_mode_t LIS331_power_mode_get(void);
esp_err_t LIS331_data_rate_set(LIS331_data_rate_t val);
LIS331_data_rate_t LIS331_data_rate_get(void);
esp_err_t LIS331_boot(void);
esp_err_t LIS331_hp_filter_set(LIS331_hp_mode_t val);
LIS331_hp_mode_t LIS331_hp_filter_get(void);
esp_err_t LIS331_hp_en_set(uint8_t interup, bool val);
bool LIS331_hp_en_get(uint8_t interup);
esp_err_t LIS331_hp_cutoff_set(LIS331_hp_cutoff_t val);
LIS331_hp_cutoff_t LIS331_hp_cutoff_get(void);
