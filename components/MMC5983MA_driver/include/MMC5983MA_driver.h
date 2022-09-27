#pragma once

#include "SPI_driver.h"
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "esp_system.h"


typedef struct{
	float magX;
	float magY;
	float magZ;
} MMC5983MA_meas_t;

typedef struct {
	int32_t Xraw;
	int32_t Yraw;
	int32_t Zraw;

	float gainX;
	float gainY;
	float gainZ;

	float offsetX;
	float offsetY;
	float offsetZ;

	MMC5983MA_meas_t meas;
} MMC5983MA_t;


typedef struct
  {
    uint8_t internalControl0;
    uint8_t internalControl1;
    uint8_t internalControl2;
    uint8_t internalControl3;
  } controlBitMemory_t;

esp_err_t MMC5983MA_init();
esp_err_t MMC5983MA_readMeas();
esp_err_t MMC5983MA_getMeas(MMC5983MA_meas_t * meas);

#define MMC_X_OUT_0_REG     0x00
#define MMC_X_OUT_1_REG     0x01
#define MMC_Y_OUT_0_REG     0x02
#define MMC_Y_OUT_1_REG     0x03
#define MMC_Z_OUT_0_REG     0x04
#define MMC_Z_OUT_1_REG     0x05
#define MMC_XYZ_OUT_2_REG   0x06
#define MMC_T_OUT_REG       0x07
#define MMC_STATUS_REG      0x08
#define MMC_INT_CTRL_0_REG  0x09
#define MMC_INT_CTRL_1_REG  0x0a
#define MMC_INT_CTRL_2_REG  0x0b
#define MMC_INT_CTRL_3_REG  0x0C
#define MMC_PROD_ID_REG     0x2F
#define MMC_DUMMY           0x00

// Constants definitions
#define MMC_I2C_ADDR        0x30
#define MMC_PROD_ID         0x30

// Bits definitions
#define MMC_MEAS_M_DONE			(1 << 0)
#define MMC_MEAS_T_DONE			(1 << 1)
#define MMC_OTP_READ_DONE		(1 << 4)
#define MMC_TM_M				(1 << 0)
#define MMC_TM_T				(1 << 1)
#define MMC_INT_MEAS_DONE_EN	(1 << 2)
#define MMC_SET_OPERATION		(1 << 3)
#define MMC_RESET_OPERATION		(1 << 4)
#define MMC_AUTO_SR_EN			(1 << 5)
#define MMC_OTP_READ			(1 << 6)
#define MMC_BW0					(1 << 0)
#define MMC_BW1					(1 << 1)
#define MMC_X_INHIBIT			(1 << 2)
#define MMC_YZ_INHIBIT			(3 << 3)
#define MMC_SW_RST				(1 << 7)
#define MMC_CM_FREQ_0			(1 << 0)
#define MMC_CM_FREQ_1			(1 << 1)
#define MMC_CM_FREQ_2			(1 << 2)
#define MMC_CMM_EN				(1 << 3)
#define MMC_PRD_SET_0			(1 << 4)
#define MMC_PRD_SET_1			(1 << 5)
#define MMC_PRD_SET_2			(1 << 6)
#define MMC_EN_PRD_SET			(1 << 7)
#define MMC_ST_ENP				(1 << 1)
#define MMC_ST_ENM				(1 << 2)
#define MMC_SPI_3W				(1 << 6)
#define MMC_X2_MASK				(3 << 6)
#define MMC_Y2_MASK				(3 << 4)
#define MMC_Z2_MASK				(3 << 2)
#define MMC_XYZ_0_SHIFT			10
#define MMC_XYZ_1_SHIFT			2
/*
typedef enum SF_MMC5983MA_ERROR
{
  NONE,
  I2C_INITIALIZATION_ERROR,
  SPI_INITIALIZATION_ERROR,
  INVALID_DEVICE,

}SF_MMC5983MA_ERROR;
*/
typedef enum mmc5983ma_cm_freq
{
	MMC5983MA_FREQ_1HZ = 1,
	MMC5983MA_FREQ_25HZ = 10,
	MMC5983MA_FREQ_75HZ = 20,
	MMC5983MA_FREQ_100HZ = 50,
	MMC5983MA_FREQ_250HZ = 100,
	MMC5983MA_FREQ_500HZ = 200,
	MMC5983MA_FREQ_1000HZ = 1000,
	MMC5983MA_FREQ_2000HZ = 0

} mmc5983ma_cm_freq_t;

typedef enum mmc5983ma_band
{
	MMC5983MA_BAND_100 = 100,
	MMC5983MA_BAND_200 = 200,
	MMC5983MA_BAND_400 = 400,
	MMC5983MA_BAND_800 = 800

} mmc5983ma_band_t;
