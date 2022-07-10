#include "esp_err.h"

typedef enum{
    SPI_SLAVE_MS5607,
    SPI_SLAVE_LIS331,
	SPI_SLAVE_LSM6DSO32,
	SPI_SLAVE_FLASH,
	SPI_SLAVE_MMC5983MA
} spi_slave_t;

esp_err_t SPI_init(uint32_t frequency);
esp_err_t SPI_RW(spi_slave_t slave, uint8_t * data_out, uint8_t * data_in, uint16_t length);
