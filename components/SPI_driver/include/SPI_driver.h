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

/**
 * @brief Initialize SPI component
 *
 * @return esp_err_t
 *  - ESP_OK: Success
 *  - Others: Fail
 */
esp_err_t SPI_init();

/**
 * @brief Configure new SPI device
 *
 * @param handle 
 * @param CS_pin Chip select pin for given device
 * @param clock_mhz Device SPI clock speed
 * @param queue_size Queue size for given SPI peripherial device
 * @param cmd_bits Number of command bits sent to device
 * @param addr_bits Number of address  bits sent to device
 * @return esp_err_t
 *  - ESP_OK: Success
 *  - Others: Fail
 */
esp_err_t SPI_registerDevice(spi_dev_handle_t *handle, int CS_pin, int clock_mhz, int queue_size, int cmd_bits, int addr_bits);


/**
 * @brief Check if SPI initialized properly 
 *
 * @return esp_err_t
 *  - ESP_OK: Success
 *  - Others: Fail
 */
esp_err_t SPI_checkInit();

/**
 * @brief Perform SPI transaction with selected devic
 *
 * @param handle 
 * @param cmd Command sent to the device
 * @param addr SPI peripheral device address 
 * @param tx_buf Transmitted buffer
 * @param rx_buf Received buffer
 * @param payload_len Length of payload sent
 * @return esp_err_t
 *  - ESP_OK: Success
 *  - Others: Fail
 */
esp_err_t SPI_transfer(spi_dev_handle_t handle, uint8_t cmd, uint8_t addr, uint8_t * tx_buf, uint8_t * rx_buf, int payload_len);
 