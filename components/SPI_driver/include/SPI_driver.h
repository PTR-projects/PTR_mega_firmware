#pragma once
#include "esp_err.h"
#include "BOARD.h"
#include <driver/spi_master.h>

esp_err_t SPI_init();
esp_err_t SPI_registerDevice(spi_device_handle_t * handle, int CS_pin, int clock_mhz, int queue_size, int cmd_bits, int addr_bits);
esp_err_t SPI_checkInit();
