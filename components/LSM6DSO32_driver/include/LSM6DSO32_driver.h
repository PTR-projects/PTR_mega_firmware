#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <driver/spi_master.h>


#define CTRL1_XL 0x10
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

int32_t gyroRes = DEFAULT_GYRO_RES;
int32_t aclRes = DEFAULT_ACL_RES;
uint8_t settings3c = 0b01000010;
uint8_t settingsFIFO = 0;
uint8_t defaultSettingsAcl = 0b00000000;
uint8_t defaultSettingsGyro = 0b00000000;
volatile uint8_t rawData[14];

const uint8_t gyroOdr[11];
const uint8_t aclOdr[12];

const uint8_t aclScale[4];
const uint8_t gyroScale[5];

esp_err_t SPI_write(uint8_t* dataArr, uint8_t len, uint8_t EN_PIN);
esp_err_t SPI_read(uint8_t* dataArr, uint8_t len, uint8_t EN_PIN);
esp_err_t LSM_begin(uint8_t EN_PIN);
esp_err_t LSM_gyroSettings(uint8_t resolution, uint8_t ODR, uint8_t LPF, uint8_t EN_PIN);
esp_err_t LSM_getRawData(uint8_t* rawData, uint8_t EN_PIN);
esp_err_t LSM_getData(uint8_t* sensorData, uint8_t EN_PIN);
esp_err_t softReset(uint8_t EN_PIN);

