#ifndef LSM6DSO32_FIFO_H
#define LSM6DSO32_FIFO_H
#include "esp_err.h"

/**
 * @brief LSM6DSO32 fifoBuffer.
 */
#define LSM6DS_FIFO_BATCH_SIZE 16 //To be moved to config

typedef struct{
			uint8_t tag;
			int16_t dataOutRaw[3];
} LSM6DSO32_fifo_data_t;

esp_err_t LSM6DSO32_configure_fifo(uint8_t sensor);
esp_err_t LSM6DSO32_readFIFOByID(uint8_t sensor);
esp_err_t parse_gyro_data(const uint8_t sampleNum, int32_t *gyroDataRaw);
esp_err_t parse_acc_data(const uint8_t sampleNum, int32_t *accDataRaw);
esp_err_t calc_acc(uint8_t sensor, int32_t *rawData);
esp_err_t calc_gyro(uint8_t sensor, int32_t *rawData);
esp_err_t collect_gyro_data(int32_t *gyroDataRaw, int16_t *sampleValue);
esp_err_t collect_acc_data(int32_t *accDataRaw, int16_t *sampleValue);

#endif // LSM6DSO32_FIFO_H