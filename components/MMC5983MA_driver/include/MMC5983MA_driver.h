#pragma once
#ifndef _MMC5983MA_CONSTANTS_
#define _MMC5983MA_CONSTANTS_
#include "SPI_driver.h"
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
typedef struct {

} MMC5983MA_t;


typedef struct
  {
    uint8_t internalControl0;
    uint8_t internalControl1;
    uint8_t internalControl2;
    uint8_t internalControl3;
  } controlBitMemory_t;

esp_err_t MMC5983MA_init();

static const uint8_t X_OUT_0_REG    = 0x0;
static const uint8_t X_OUT_1_REG    = 0x01;
static const uint8_t Y_OUT_0_REG    = 0x02;
static const uint8_t Y_OUT_1_REG    = 0x03;
static const uint8_t Z_OUT_0_REG    = 0x04;
static const uint8_t Z_OUT_1_REG    = 0x05;
static const uint8_t XYZ_OUT_2_REG  = 0x06;
static const uint8_t T_OUT_REG      = 0x07;
static const uint8_t STATUS_REG     = 0x08;
static const uint8_t INT_CTRL_0_REG = 0x09;
static const uint8_t INT_CTRL_1_REG = 0x0a;
static const uint8_t INT_CTRL_2_REG = 0x0b;
static const uint8_t INT_CTRL_3_REG = 0x0c;
static const uint8_t PROD_ID_REG    = 0x2f;
static const uint8_t DUMMY          = 0x0;

// Constants definitions
static const uint8_t I2C_ADDR       = 0x30;
static const uint8_t PROD_ID        = 0x30;

// Bits definitions
#define MEAS_M_DONE                 (1 << 0)
#define MEAS_T_DONE                 (1 << 1)
#define OTP_READ_DONE               (1 << 4)
#define TM_M                        (1 << 0)
#define TM_T                        (1 << 1)
#define INT_MEAS_DONE_EN            (1 << 2)
#define SET_OPERATION               (1 << 3)
#define RESET_OPERATION             (1 << 4)
#define AUTO_SR_EN                  (1 << 5)
#define OTP_READ                    (1 << 6)
#define BW0                         (1 << 0)
#define BW1                         (1 << 1)
#define X_INHIBIT                   (1 << 2)
#define YZ_INHIBIT                  (3 << 3)
#define SW_RST                      (1 << 7)
#define CM_FREQ_0                   (1 << 0)
#define CM_FREQ_1                   (1 << 1)
#define CM_FREQ_2                   (1 << 2)
#define CMM_EN                      (1 << 3)
#define PRD_SET_0                   (1 << 4)
#define PRD_SET_1                   (1 << 5)
#define PRD_SET_2                   (1 << 6)
#define EN_PRD_SET                  (1 << 7)
#define ST_ENP                      (1 << 1)
#define ST_ENM                      (1 << 2)
#define SPI_3W                      (1 << 6)
#define X2_MASK                     (3 << 6)
#define Y2_MASK                     (3 << 4)
#define Z2_MASK                     (3 << 2)
#define XYZ_0_SHIFT                 10
#define XYZ_1_SHIFT                 2

typedef enum class
{
  NONE,
  I2C_INITIALIZATION_ERROR,
  SPI_INITIALIZATION_ERROR,
  INVALID_DEVICE,

}SF_MMC5983MA_ERROR;

esp_err_t MMC5983MA_read(uint8_t  address, uint8_t * buf, uint8_t len);
uint8_t MMC5983MA_readSingleByte(uint8_t  address);
esp_err_t MMC5983MA_command(uint8_t command);
esp_err_t MMC5983MA_writeSingleByte(uint8_t adress, uint8_t value);
esp_err_t MMC5983MA_init();
esp_err_t MMC5983MA_setRegisterBit(uint8_t registerAddress, uint8_t bitMask);
void MMC5983MA_setControlBit(uint8_t registerAddress, const uint8_t bitMask);
void MMC5983MA_clearControlBit(uint8_t registerAddress, const uint8_t bitMask);
bool MMC5983MA_isControlBitSet(uint8_t registerAddress, const uint8_t bitMask);
int MMC5983MA_getTemperature();
void MMC5983MA_softReset();
void MMC5983MA_enableInterrupt();
void MMC5983MA_disableInterrupt();
bool MMC5983MA_isInterruptEnabled();
void MMC5983MA_enable3WireSPI();
void MMC5983MA_disable3WireSPI();
bool MMC5983MA_is3WireSPIEnabled();
void MMC5983MA_performSetOperation();
void MMC5983MA_performResetOperation();
void MMC5983MA_enableAutomaticSetReset();
void MMC5983MA_disableAutomaticSetReset();
bool MMC5983MA_isAutomaticSetResetEnabled();
void MMC5983MA_enableXChannel();
void MMC5983MA_disableXChannel();
bool MMC5983MA_isXChannelEnabled();
void MMC5983MA_enableYZChannels();
void MMC5983MA_disableYZChannels();
bool MMC5983MA_areYZChannelsEnabled();
void MMC5983MA_setFilterBandwidth(uint16_t bandwidth);
uint16_t MMC5983MA_getFilterBandwith();
void MMC5983MA_enableContinuousMode();
void MMC5983MA_disableContinuousMode();
bool MMC5983MA_isContinuousModeEnabled();
void MMC5983MA_setContinuousModeFrequency(uint16_t frequency);
uint16_t MMC5983MA_getContinuousModeFrequency();
void MMC5983MA_enablePeriodicSet();
void MMC5983MA_disablePeriodicSet();
bool MMC5983MA_isPeriodicSetEnabled();
void MMC5983MA_setPeriodicSetSamples(const uint16_t numberOfSamples);
uint16_t MMC5983MA_getPeriodicSetSamples();
void MMC5983MA_applyExtraCurrentPosToNeg();
void MMC5983MA_removeExtraCurrentPosToNeg();
bool MMC5983MA_isExtraCurrentAppliedPosToNeg();
void MMC5983MA_applyExtracurrentNegToPos();
void MMC5983MA_removeExtracurrentNegToPos();
bool MMC5983MA_isExtraCurrentAppliedNegToPos();
uint32_t MMC5983MA_getMeasurementX();
uint32_t MMC5983MA_getMeasurementY();
uint32_t MMC5983MA_getMeasurementZ();




#endif
