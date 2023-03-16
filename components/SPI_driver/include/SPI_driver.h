#pragma once
#include "esp_err.h"
#include "BOARD.h"
#include <driver/spi_master.h>
typedef spi_device_handle_t spi_dev_handle_t;

esp_err_t SPI_init();
esp_err_t SPI_registerDevice(spi_dev_handle_t *handle, int CS_pin, int clock_mhz, int queue_size, int cmd_bits, int addr_bits);
esp_err_t SPI_checkInit();
esp_err_t SPI_transfer(spi_dev_handle_t handle, uint8_t cmd, uint8_t addr, uint8_t * tx_buf, uint8_t * rx_buf, int payload_len);
