/**
  ******************************************************************************
  * @file    LIS331_driver.c
  * @author  Kretu
  * @brief   LIS331 driver file
  ******************************************************************************
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "LIS331_driver.h"
#include <string.h>
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_err.h"







#define INIT_TIME       5 		//ms


static LIS331_t LIS331_d;



esp_err_t LIS331_spi_init(void)
{
	esp_err_t ret = ESP_OK;

/*  SPI BUS INITIALIZATION */
// Uncomment if not initialised elsewhere
/*
	spi_bus_config_t buscfg={
		.miso_io_num   = SPI_MISO_PIN,
		.mosi_io_num   = SPI_MOSI_PIN,
		.sclk_io_num   = SPI_SCK_PIN,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,

	};

	ret = spi_bus_initialize(SPI_BUS, &buscfg, SPI_DMA_DISABLED); //Initialize the SPI bus (prev: SPI_DMA_CH_AUTO)
	ESP_ERROR_CHECK(ret);

*/

/* CONFIGURE SPI DEVICE */

spi_device_interface_config_t lis331_spi_config = {
			.mode           =  0,
			.spics_io_num   = SPI_CS_PIN,
			.clock_speed_hz =  1 * 1000 * 1000,
			.queue_size     =  1,
			.command_bits = 2,
			.address_bits = 6,

		};

ret = spi_bus_add_device(SPI_BUS, &lis331_spi_config, &spi_dev_handle_LIS331);
return ret;
}



esp_err_t LIS331_read(uint8_t addr, uint8_t * data_in, uint16_t length){

	esp_err_t ret = ESP_OK;
	spi_transaction_t trans;
	memset(&trans, 0x00, sizeof(trans));
	trans.length = (8 + (8 * length));
	trans.rxlength = 8 * length;
	trans.cmd = CMD_READ;
	trans.addr = addr;
	trans.rx_buffer = data_in;

	spi_device_acquire_bus(spi_dev_handle_LIS331, portMAX_DELAY);
	if (spi_device_polling_transmit(spi_dev_handle_LIS331, &trans) != ESP_OK)
	{
		ESP_LOGE("SPI DRIVER", "%s(%d): spi transmit failed", __FUNCTION__, __LINE__);
		ret = ESP_FAIL;
	}
	spi_device_release_bus(spi_dev_handle_LIS331);

	return ret;
}

uint8_t LIS331_read_single(uint8_t addr){

	uint8_t buf[1] = {0};
	spi_transaction_t trans;
	memset(&trans, 0x00, sizeof(trans));
	trans.length = 16;
	trans.rxlength = 8;
	trans.cmd = CMD_READ;
	trans.addr = addr;
	trans.rx_buffer = buf;

	spi_device_acquire_bus(spi_dev_handle_LIS331, portMAX_DELAY);
	if (spi_device_polling_transmit(spi_dev_handle_LIS331, &trans) != ESP_OK)
	{
		ESP_LOGE("SPI DRIVER", "%s(%d): spi transmit failed", __FUNCTION__, __LINE__);
	}
	spi_device_release_bus(spi_dev_handle_LIS331);

	return buf[0];
}

esp_err_t LIS331_write(uint8_t addr, uint8_t data_out){

	uint8_t buff[1] = {data_out};
	esp_err_t ret = ESP_OK;
	spi_transaction_t trans;
	memset(&trans, 0x00, sizeof(trans));
	trans.length = (8 + 8);
	trans.rxlength = 8 ;
	trans.cmd = CMD_WRITE;
	trans.addr = addr;
	trans.tx_buffer = buff;


	spi_device_acquire_bus(spi_dev_handle_LIS331, portMAX_DELAY);
	if (spi_device_polling_transmit(spi_dev_handle_LIS331, &trans) != ESP_OK)
	{
		ESP_LOGE("SPI DRIVER", "%s(%d): spi transmit failed", __FUNCTION__, __LINE__);
		ret = ESP_FAIL;
	}
	spi_device_release_bus(spi_dev_handle_LIS331);

	return ret;
}

esp_err_t LIS331_set(uint8_t reg_addr, uint8_t cmd_mask, uint8_t cmd_value)
{
	uint8_t reg = 0;
	reg = LIS331_read_single(reg_addr);
	reg &= ~cmd_mask;
	reg |= cmd_value;
	LIS331_write(reg_addr, reg);


return ESP_OK;

}

uint8_t LIS331_get(uint8_t reg_addr, uint8_t cmd_mask)
{

	uint8_t reg = 0;
	reg = LIS331_read_single(reg_addr);


return (reg & cmd_mask);

}


uint8_t LIS331_WhoAmI(void)
{
	uint8_t whois = 0;
	LIS331_read(LIS331_WHO_AM_I, &whois, 1);
	return whois;
}

esp_err_t LIS331_init(LIS331_type_t type)
{
	LIS331_spi_init();
	printf("Inicjalizacja SPI ok");
	uint8_t range = 0;
	if((type == LIS331_IC_2G) || (type == LIS331HH_IC_6G) || (type == LIS331_IC_100G))
		range = 0;

	if((type == LIS331_IC_4G) || (type == LIS331HH_IC_12G) || (type == LIS331_IC_200G))
		range = 1;

	if((type == LIS331_IC_8G) || (type == LIS331HH_IC_24G) || (type == LIS331_IC_400G))
		range = 3;


	LIS331_write(LIS331_CTRL_REG1, 0x37);	//XYZ enabled, Normal mode, 400Hz ODR
	LIS331_write(LIS331_CTRL_REG4, (range << 4) | BDU_MASK);


	LIS331_d.sensor_range = type;

	return ESP_OK;	//ESP_FAIL

}


esp_err_t LIS331_x_axis_set(bool val)
{
	if (!val){
		LIS331_set(LIS331_CTRL_REG1, XEN_MASK , 0x0);
	}
	else
	{
		LIS331_set(LIS331_CTRL_REG1, XEN_MASK , XEN_MASK);
	}
return ESP_OK;
}

bool LIS331_x_axis_get(void)
{
	return LIS331_get(LIS331_CTRL_REG1, XEN_MASK) == XEN_MASK;
}


esp_err_t LIS331_y_axis_set(bool val)
{
	if (!val){
		LIS331_set(LIS331_CTRL_REG1, YEN_MASK , 0x0);
	}
	else
	{
		LIS331_set(LIS331_CTRL_REG1, YEN_MASK , YEN_MASK);
	}
return ESP_OK;
}

bool LIS331_y_axis_get(void)
{
	return LIS331_get(LIS331_CTRL_REG1, YEN_MASK) == YEN_MASK;
}

esp_err_t LIS331_z_axis_set(bool val)
{
	if (!val){
		LIS331_set(LIS331_CTRL_REG1, ZEN_MASK , 0x0);
	}
	else
	{
		LIS331_set(LIS331_CTRL_REG1, ZEN_MASK , ZEN_MASK);
	}
return ESP_OK;
}

bool LIS331_z_axis_get(void)
{
	return LIS331_get(LIS331_CTRL_REG1, ZEN_MASK) == ZEN_MASK;
}





esp_err_t LIS331_power_mode_set(LIS331_power_mode_t val)
{
	LIS331_set(LIS331_CTRL_REG1, POWER_MODE_MASK , val);

  return ESP_OK;
}

LIS331_power_mode_t LIS331_power_mode_get(void)
{
	return LIS331_get(LIS331_CTRL_REG1, POWER_MODE_MASK);
}

esp_err_t LIS331_data_rate_set(LIS331_data_rate_t val)
{
	LIS331_set(LIS331_CTRL_REG1, DATA_RATE_MASK , val);

  return ESP_OK;
}

LIS331_data_rate_t LIS331_data_rate_get(void)
{
	return LIS331_get(LIS331_CTRL_REG1, DATA_RATE_MASK);
}



esp_err_t LIS331_xyz_acc_raw_get(void)
{
	printf("Raw read start\n");

  LIS331_read(LIS331_OUT_X_L, LIS331_d.raw, 6);
  LIS331_d.accX_raw = LIS331_d.accX_raw >> 4;
  LIS331_d.accY_raw = LIS331_d.accY_raw >> 4;
  LIS331_d.accZ_raw = LIS331_d.accZ_raw >> 4;
  printf("Raw read ok\n");
  return ESP_OK;
}

esp_err_t LIS331_xyz_acc_calc(void)
{
	LIS331_xyz_acc_raw_get();
	float range = 2 * LIS331_d.sensor_range;
	if(range == 0.0f)
		return ESP_ERR_INVALID_ARG;
	LIS331_d.meas.accX = (LIS331_d.accX_raw)*(range/4096.0f) - LIS331_d.accXoffset;
	LIS331_d.meas.accY = (LIS331_d.accY_raw)*(range/4096.0f) - LIS331_d.accYoffset;
	LIS331_d.meas.accZ = (LIS331_d.accZ_raw)*(range/4096.0f) - LIS331_d.accZoffset;
	return ESP_OK;
}

esp_err_t LIS331_getMeasurementXYZ(float* X, float* Y, float* Z)
{
	LIS331_xyz_acc_calc();
	*X = LIS331_d.meas.accX;
	*Y = LIS331_d.meas.accY;
	*Z = LIS331_d.meas.accZ;
	return ESP_OK;
}

esp_err_t LIS331_boot(void)
{
	LIS331_set(LIS331_CTRL_REG2, BOOT_MASK , BOOT_MASK);

  return ESP_OK;
}


esp_err_t LIS331_hp_filter_set(LIS331_hp_mode_t val)
{
	LIS331_set(LIS331_CTRL_REG2, HP_MODE_MASK , val);

  return ESP_OK;
}

LIS331_hp_mode_t LIS331_hp_filter_get(void)
{
	return LIS331_get(LIS331_CTRL_REG2, HP_MODE_MASK);
}

esp_err_t LIS331_hp_en_set(uint8_t interup, bool val)
{
	switch(interup){
	case 1:
		if (!val){
			LIS331_set(LIS331_CTRL_REG2, HP_EN1_MASK, 0);
		}
		else
		{
			LIS331_set(LIS331_CTRL_REG2, HP_EN1_MASK, HP_EN1_MASK);
		}
		break;

	case 2:
		if (!val){
			LIS331_set(LIS331_CTRL_REG2, HP_EN2_MASK, 0);
		}
		else
		{
			LIS331_set(LIS331_CTRL_REG2, HP_EN2_MASK,HP_EN2_MASK);
		}
		break;
	default:
		return ESP_ERR_INVALID_ARG;
	}
  return ESP_OK;
}

bool LIS331_hp_en_get(uint8_t interup)
{
	switch(interup){
	case 1:
		return LIS331_get(LIS331_CTRL_REG2, HP_EN1_MASK);

	case 2:
		return LIS331_get(LIS331_CTRL_REG2, HP_EN2_MASK);
	default:
		return ESP_ERR_INVALID_ARG;
	}
}


esp_err_t LIS331_hp_cutoff_set(LIS331_hp_cutoff_t val)
{
	LIS331_set(LIS331_CTRL_REG2, HP_CUTOFF_MASK , val);

  return ESP_OK;
}

LIS331_hp_cutoff_t LIS331_hp_cutoff_get(void)
{
	return LIS331_get(LIS331_CTRL_REG2, HP_CUTOFF_MASK);
}


esp_err_t LIS331_bdu_set(bool val)
{
	if (!val){
		LIS331_set(LIS331_CTRL_REG4, BDU_MASK, 0);
	}
	else{
		LIS331_set(LIS331_CTRL_REG4, BDU_MASK, BDU_MASK);
	}
	return ESP_OK;
}

bool LIS331_bdu_get(void)
{
	return LIS331_get(LIS331_CTRL_REG4, BDU_MASK) == BDU_MASK;
}

esp_err_t LIS331_ble_set(bool val)
{
	if (!val){
		LIS331_set(LIS331_CTRL_REG4, BLE_MASK, 0);
	}
	else{
		LIS331_set(LIS331_CTRL_REG4, BLE_MASK, BLE_MASK);
	}
	return ESP_OK;
}

bool LIS331_ble_get(void)
{
	return LIS331_get(LIS331_CTRL_REG4, BLE_MASK) == BLE_MASK;
}



esp_err_t LIS331_range_set(LIS331_range_t val)
{
	LIS331_set(LIS331_CTRL_REG4, FS_MASK , val);
	switch(val){
	case LIS331_RANGE_100G:
		LIS331_d.sensor_range = 100;
		break;
	case LIS331_RANGE_200G:
		LIS331_d.sensor_range = 200;
		break;
	case LIS331_RANGE_400G:
		LIS331_d.sensor_range = 400;
		break;
	default:
		return ESP_ERR_INVALID_ARG;
	}

  return ESP_OK;
}

LIS331_range_t LIS331_range_get(void)
{
	return LIS331_get(LIS331_CTRL_REG4, FS_MASK);
}

