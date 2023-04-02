#pragma once
#include "esp_err.h"
#include "BOARD.h"
#include <driver/spi_master.h>
typedef spi_device_handle_t spi_dev_handle_t;

typedef enum {
	SPI_SCK_1MHZ = 1,
	SPI_SCK_2MHZ = 2,
	SPI_SCK_3MHZ = 3,
	SPI_SCK_4MHZ = 4,
	SPI_SCK_5MHZ = 5,
	SPI_SCK_6MHZ = 6,
	SPI_SCK_7MHZ = 7,
	SPI_SCK_8MHZ = 8,
	SPI_SCK_9MHZ = 9,
	SPI_SCK_10MHZ = 10,
	SPI_SCK_11MHZ = 11,
	SPI_SCK_12MHZ = 12,
	SPI_SCK_13MHZ = 13,
	SPI_SCK_14MHZ = 14,
	SPI_SCK_15MHZ = 15,
	SPI_SCK_16MHZ = 16,
	SPI_SCK_17MHZ = 17,
	SPI_SCK_18MHZ = 18,
	SPI_SCK_19MHZ = 19,
	SPI_SCK_20MHZ = 20
} SPI_sck_freq_t;

esp_err_t SPI_init();
esp_err_t SPI_registerDevice(spi_dev_handle_t *handle, int CS_pin, int clock_mhz, int queue_size, int cmd_bits, int addr_bits);
esp_err_t SPI_checkInit();
esp_err_t SPI_transfer(spi_dev_handle_t handle, uint8_t cmd, uint8_t addr, uint8_t * tx_buf, uint8_t * rx_buf, int payload_len);
