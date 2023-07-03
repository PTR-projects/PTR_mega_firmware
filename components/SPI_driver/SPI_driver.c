#include <stdio.h>
#include <string.h>
#include <driver/spi_master.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "BOARD.h"
#include "SPI_driver.h"

static const char *TAG = "SPI_driver";
static esp_err_t SPI_init_done = ESP_ERR_NOT_FINISHED;
 
esp_err_t SPI_init(){
	// -------------- SPI init --------------------------------------------
	spi_bus_config_t buscfg = {
		.flags 		   = SPICOMMON_BUSFLAG_MASTER,
		.miso_io_num   = SPI_MISO_PIN,
		.mosi_io_num   = SPI_MOSI_PIN,
		.sclk_io_num   = SPI_SCK_PIN,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
	};

	 //Initialize the SPI bus
	ESP_RETURN_ON_ERROR(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO), TAG, "spi_bus_init failed");

	SPI_init_done = ESP_OK;
	return ESP_OK;
}

esp_err_t SPI_registerDevice(spi_dev_handle_t *handle, int CS_pin, int clock_mhz, int queue_size, int cmd_bits, int addr_bits){
	ESP_RETURN_ON_FALSE(((cmd_bits+addr_bits)%8) == 0, ESP_ERR_INVALID_ARG, TAG, "SPI_register - wrong ADDR or CMD length");

	/* CONFIGURE SPI DEVICE */
	spi_device_interface_config_t spi_device_config = {
		.mode           = 0,
		.spics_io_num   = CS_pin,
		.clock_speed_hz = clock_mhz * 1000 * 1000,
		.queue_size     = queue_size,
		.command_bits 	= cmd_bits,
		.address_bits 	= addr_bits,
		.dummy_bits		= 0,
		.duty_cycle_pos	= 0,
		.cs_ena_pretrans	= 0,
		.cs_ena_posttrans	= 0,
		.input_delay_ns	= 0,
		.flags			= 0,
		.pre_cb			= NULL,
		.post_cb		= NULL
	};

	ESP_RETURN_ON_ERROR(spi_bus_add_device(SPI2_HOST, &spi_device_config, handle), TAG, "Failed to add new device, CS_pin %i", CS_pin);
//	esp_err_t ret = spi_bus_add_device(SPI2_HOST, &spi_device_config, handle);
//	ESP_LOGV(TAG, "Add CS %i returned: %i", CS_pin, (int)ret);

	return ESP_OK;
}

esp_err_t SPI_transfer(spi_dev_handle_t handle, uint8_t cmd, uint8_t addr, uint8_t * tx_buf, uint8_t * rx_buf, int payload_len){
	ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "SPI_transfer - handle is NULL");

	esp_err_t ret = ESP_OK;
	spi_transaction_t trans;
	memset(&trans, 0x00, sizeof(trans));
	trans.length 	= 8 * payload_len;	// Length of transaction minus CMD and ADDR
	trans.rxlength 	= 8 * payload_len;
	trans.cmd 		= cmd;
	trans.addr 		= addr;
	trans.rx_buffer = rx_buf;
	trans.tx_buffer = tx_buf;

	if(spi_device_acquire_bus(handle, portMAX_DELAY) == ESP_OK){  //TODO <<--------------------------------------------- dać jakiś limit na timeout???
		if (spi_device_polling_transmit(handle, &trans) != ESP_OK) {
			ESP_LOGE("SPI DRIVER", "%s(%d): spi transmit failed", __FUNCTION__, __LINE__);
			ret = ESP_FAIL;
		}
		spi_device_release_bus(handle);
	}

	return ret;
}

esp_err_t SPI_longTransfer(){

	return ESP_OK;
}

esp_err_t SPI_checkInit(){
	return SPI_init_done;
}
