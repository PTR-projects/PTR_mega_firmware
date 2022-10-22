#include <stdio.h>
#include "esp_err.h"
#include "SPI_driver.h"
#include "LSM6DSO32_driver.h"

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
//2 3 4 5 6 7 8 9 A B C D

#define OUTX_L_G 0x22
#define DEFAULT_GYRO_RES 5
#define DEFAULT_ACL_RES 5
int32_t gyroRes DEFAULT_GYRO_RES;
int32_t aclRes DEFAULT_ACL_RES;
uint8_t settings3c = 0b01000010;
uint8_t settingsFIFO = 0;
uint8_t defaultSettingsAcl = 0b00000000;
uint8_t defaultSettingsGyro = 0b00000000;
volatile uint8_t rawData[14];

const uint8_t gyroOdr[11];
const uint8_t aclOdr[12];

const uint8_t aclScale[4];
const uint8_t gyroScale[5];

void SPI_write(uint8_t* dataArr, uint8_t len, uint8_t EN_PIN);

void SPI_read(uint8_t* dataArr, uint8_t len, uint8_t EN_PIN);

bool LSM_begin(uint8_t EN_PIN);

void LSM_gyroSettings(uint8_t resolution, uint8_t ODR, uint8_t LPF, uint8_t EN_PIN);

void LSM_getRawData(uint8_t* rawData, uint8_t EN_PIN);

void LSM_getData(uint8_t* sensorData, uint8_t EN_PIN);

void softReset(uint8_t EN_PIN);

