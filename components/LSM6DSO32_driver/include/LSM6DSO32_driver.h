#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <driver/spi_master.h>


#define CTRL1_XL 0x10
#define CTRL8_XL 0x17
#define CTRL2_G  0x11
#define CTRL3_C  0x12
#define CTRL7_G 0x16
#define CTRL6_C 0x15
#define FIFO_CTRL3 0x08
#define FIFO_CTRL4 0x09
#define FIFO_CTRL1 0x06
#define FIFO_CTRL2 0x07
#define FIFO_CTRL4 0x09
#define FIFO_CTRL_5 0x0A
#define MASTER_CONFIG 0x1A
#define WAKE_UP_SRC 0x1B
#define CS_PIN 5

#define OUTX_L_G 0x22
#define DEFAULT_GYRO_RES 5
#define DEFAULT_ACL_RES 5

#define LSM6_ODR_12_5HZ 1
#define LSM6_ODR_26HZ 2
#define LSM6_ODR_52HZ 3
#define LSM6_ODR_104HZ 4
#define LSM6_ODR_208HZ 5
#define LSM6_ODR_416HZ 6 
#define LSM6_ODR_833HZ 7
#define LSM6_ODR_1600HZ 8
#define LSM6_ODR_3330HZ 9
#define LSM6_ODR_6660HZ 10

#define LSM6_RESOLUTION_125DPS 0
#define LSM6_RESOLUTION_250DPS 1
#define LSM6_RESOLUTION_500DPS 2
#define LSM6_RESOLUTION_1000DPS 3 
#define LSM6_RESOLUTION_2000DPS 4






#define SPI_BUS SPI2_HOST
#define CMD_READ 0x03
#define CMD_WRITE 0x01


static spi_device_handle_t spi_dev_handle_LSM6;

typedef struct{
	float temp;

	float accX;
	float accY;
	float accZ;

	float gyroX;
	float gyroY;
	float gyroZ;
} LSM6DS_meas_t;

typedef struct{

} LSM6DSO32_t;

static inline int32_t gyroRes = DEFAULT_GYRO_RES;
static inline int32_t aclRes = DEFAULT_ACL_RES;
static inline uint8_t settings3c = 0b01000010;
static inline uint8_t settingsFIFO = 0;
static inline uint8_t defaultSettingsAcl = 0b00000000;
static inline uint8_t defaultSettingsGyro = 0b00000000;


esp_err_t LSM6_begin(uint8_t EN_PIN);
esp_err_t LSM6_setGyroODR(uint16_t gyroDataRate_Hz);
esp_err_t LSM6_setGyroReadout(uint8_t gyroResolution_DPS, uint8_t gyroLPF);
esp_err_t LSM6_getRawData(uint8_t* rawData);
esp_err_t LSM6_getData(uint8_t* sensorData);
esp_err_t LSM6_softReset();
