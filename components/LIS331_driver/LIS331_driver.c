/**
  ******************************************************************************
  * @file    LIS331_driver.c
  * @author  Kretu
  * @brief   LIS331 driver file
  ******************************************************************************
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "SPI_driver.h"
#include "LIS331_driver.h"
#include <string.h>
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"

#define INIT_TIME       5 		//ms

static const char *TAG = "LIS331";
static LIS331_t LIS331_d;
static spi_dev_handle_t spi_dev_handle_LIS331;


esp_err_t LIS331_spi_init(void)
{
	if(SPI_checkInit() != ESP_OK){
		ESP_LOGE(TAG, "SPI controller not initialized! Use SPI_init() in main.c");
		return ESP_ERR_INVALID_STATE;
	}

	/* CONFIGURE SPI DEVICE */
	ESP_RETURN_ON_ERROR(SPI_registerDevice(&spi_dev_handle_LIS331, SPI_SLAVE_LIS331_PIN, 1, 1, 2, 6), TAG, "SPI register failed");

	return ESP_OK;
}

esp_err_t LIS331_read(uint8_t addr, uint8_t * data_in, uint16_t length){
	return SPI_transfer(spi_dev_handle_LIS331, LIS331_CMD_READ, addr, NULL, data_in, length);
}

uint8_t LIS331_read_single(uint8_t addr){
	uint8_t buf = 0;
	SPI_transfer(spi_dev_handle_LIS331, LIS331_CMD_READ, addr, NULL, &buf, 1);

	return buf;
}

esp_err_t LIS331_write(uint8_t addr, uint8_t data_out){
	return SPI_transfer(spi_dev_handle_LIS331, LIS331_CMD_WRITE, addr, &data_out, NULL, 1);
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
	ESP_LOGI(TAG, "LIS331 SPI init OK");
	uint8_t range = 0;
	if((type == LIS331_IC_2G) || (type == LIS331HH_IC_6G) || (type == LIS331_IC_100G))
		range = 0;

	if((type == LIS331_IC_4G) || (type == LIS331HH_IC_12G) || (type == LIS331_IC_200G))
		range = 1;

	if((type == LIS331_IC_8G) || (type == LIS331HH_IC_24G) || (type == LIS331_IC_400G))
		range = 3;

	LIS331_write(LIS331_CTRL_REG1, 0x37);	//XYZ enabled, Normal mode, 400Hz ODR
	LIS331_write(LIS331_CTRL_REG4, (range << 4) | LIS331_BDU_MASK);

	LIS331_d.sensor_range = type;

	return ESP_OK;	//ESP_FAIL
}


esp_err_t LIS331_x_axis_set(bool val)
{
	if (!val){
		LIS331_set(LIS331_CTRL_REG1, LIS331_XEN_MASK , 0x0);
	}
	else
	{
		LIS331_set(LIS331_CTRL_REG1, LIS331_XEN_MASK , LIS331_XEN_MASK);
	}
	return ESP_OK;
}

bool LIS331_x_axis_get(void)
{
	return LIS331_get(LIS331_CTRL_REG1, LIS331_XEN_MASK) == LIS331_XEN_MASK;
}


esp_err_t LIS331_y_axis_set(bool val)
{
	if (!val){
		LIS331_set(LIS331_CTRL_REG1, LIS331_YEN_MASK , 0x0);
	}
	else
	{
		LIS331_set(LIS331_CTRL_REG1, LIS331_YEN_MASK , LIS331_YEN_MASK);
	}
	return ESP_OK;
}

bool LIS331_y_axis_get(void)
{
	return LIS331_get(LIS331_CTRL_REG1, LIS331_YEN_MASK) == LIS331_YEN_MASK;
}

esp_err_t LIS331_z_axis_set(bool val)
{
	if (!val){
		LIS331_set(LIS331_CTRL_REG1, LIS331_ZEN_MASK , 0x0);
	}
	else
	{
		LIS331_set(LIS331_CTRL_REG1, LIS331_ZEN_MASK , LIS331_ZEN_MASK);
	}
	return ESP_OK;
}

bool LIS331_z_axis_get(void)
{
	return LIS331_get(LIS331_CTRL_REG1, LIS331_ZEN_MASK) == LIS331_ZEN_MASK;
}

esp_err_t LIS331_power_mode_set(LIS331_power_mode_t val)
{
	LIS331_set(LIS331_CTRL_REG1, LIS331_POWER_MODE_MASK , val);

	return ESP_OK;
}

LIS331_power_mode_t LIS331_power_mode_get(void)
{
	return LIS331_get(LIS331_CTRL_REG1, LIS331_POWER_MODE_MASK);
}

esp_err_t LIS331_data_rate_set(LIS331_data_rate_t val)
{
	LIS331_set(LIS331_CTRL_REG1, LIS331_DATA_RATE_MASK , val);

	return ESP_OK;
}

LIS331_data_rate_t LIS331_data_rate_get(void)
{
	return LIS331_get(LIS331_CTRL_REG1, LIS331_DATA_RATE_MASK);
}

esp_err_t LIS331_xyz_acc_raw_get(void)
{
	ESP_LOGV(TAG, "Raw read start\n");

  LIS331_read(LIS331_OUT_X_L, LIS331_d.raw, 6);
  LIS331_d.accX_raw = LIS331_d.accX_raw >> 4;
  LIS331_d.accY_raw = LIS331_d.accY_raw >> 4;
  LIS331_d.accZ_raw = LIS331_d.accZ_raw >> 4;
  ESP_LOGV(TAG, "Raw read ok\n");
  return ESP_OK;
}

esp_err_t LIS331_readMeas(void)
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

esp_err_t LIS331_getMeas(LIS331_meas_t * meas){
	*meas = LIS331_d.meas;

	return ESP_OK;	//ESP_FAIL
}


esp_err_t LIS331_getMeasurementXYZ(float* X, float* Y, float* Z)
{
	LIS331_readMeas();
	*X = LIS331_d.meas.accX;
	*Y = LIS331_d.meas.accY;
	*Z = LIS331_d.meas.accZ;
	return ESP_OK;
}

esp_err_t LIS331_boot(void)
{
	LIS331_set(LIS331_CTRL_REG2, LIS331_BOOT_MASK , LIS331_BOOT_MASK);

  return ESP_OK;
}


esp_err_t LIS331_hp_filter_set(LIS331_hp_mode_t val)
{
	LIS331_set(LIS331_CTRL_REG2, LIS331_HP_MODE_MASK , val);

  return ESP_OK;
}

LIS331_hp_mode_t LIS331_hp_filter_get(void)
{
	return LIS331_get(LIS331_CTRL_REG2, LIS331_HP_MODE_MASK);
}

esp_err_t LIS331_hp_en_set(uint8_t interup, bool val)
{
	switch(interup){
	case 1:
		if (!val){
			LIS331_set(LIS331_CTRL_REG2, LIS331_HP_EN1_MASK, 0);
		}
		else
		{
			LIS331_set(LIS331_CTRL_REG2, LIS331_HP_EN1_MASK, LIS331_HP_EN1_MASK);
		}
		break;

	case 2:
		if (!val){
			LIS331_set(LIS331_CTRL_REG2, LIS331_HP_EN2_MASK, 0);
		}
		else
		{
			LIS331_set(LIS331_CTRL_REG2, LIS331_HP_EN2_MASK,LIS331_HP_EN2_MASK);
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
		return LIS331_get(LIS331_CTRL_REG2, LIS331_HP_EN1_MASK);

	case 2:
		return LIS331_get(LIS331_CTRL_REG2, LIS331_HP_EN2_MASK);
	default:
		return ESP_ERR_INVALID_ARG;
	}
}


esp_err_t LIS331_hp_cutoff_set(LIS331_hp_cutoff_t val)
{
	LIS331_set(LIS331_CTRL_REG2, LIS331_HP_CUTOFF_MASK , val);

  return ESP_OK;
}

LIS331_hp_cutoff_t LIS331_hp_cutoff_get(void)
{
	return LIS331_get(LIS331_CTRL_REG2, LIS331_HP_CUTOFF_MASK);
}


esp_err_t LIS331_bdu_set(bool val)
{
	if (!val){
		LIS331_set(LIS331_CTRL_REG4, LIS331_BDU_MASK, 0);
	}
	else{
		LIS331_set(LIS331_CTRL_REG4, LIS331_BDU_MASK, LIS331_BDU_MASK);
	}
	return ESP_OK;
}

bool LIS331_bdu_get(void)
{
	return LIS331_get(LIS331_CTRL_REG4, LIS331_BDU_MASK) == LIS331_BDU_MASK;
}

esp_err_t LIS331_ble_set(bool val)
{
	if (!val){
		LIS331_set(LIS331_CTRL_REG4, LIS331_BLE_MASK, 0);
	}
	else{
		LIS331_set(LIS331_CTRL_REG4, LIS331_BLE_MASK, LIS331_BLE_MASK);
	}
	return ESP_OK;
}

bool LIS331_ble_get(void)
{
	return LIS331_get(LIS331_CTRL_REG4, LIS331_BLE_MASK) == LIS331_BLE_MASK;
}



esp_err_t LIS331_range_set(LIS331_range_t val)
{
	LIS331_set(LIS331_CTRL_REG4, LIS331_FS_MASK , val);
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
	return LIS331_get(LIS331_CTRL_REG4, LIS331_FS_MASK);
}

