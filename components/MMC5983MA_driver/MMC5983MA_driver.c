#include <stdio.h>
#include "esp_err.h"
#include "esp_log.h"
#include "MMC5983MA_driver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define READ_REG(x) (0x80 | x)
static const char* TAG = "MMC5983MA";
static controlBitMemory_t controlBitMemory;

esp_err_t MMC5983MA_read(uint8_t  address, uint8_t * buf, uint8_t len) {
	uint8_t txBuff[7] = {READ_REG(address)};
    SPI_RW(SPI_SLAVE_MMC5983MA, txBuff, buf, len+1);
    return ESP_OK;
}

uint8_t MMC5983MA_readSingleByte(uint8_t  address) {
	uint8_t txBuff[2] = {READ_REG(address)};
	uint8_t buf[2] = {0};
    SPI_RW(SPI_SLAVE_MMC5983MA, txBuff, buf, 2);
    return buf[1];
}

esp_err_t MMC5983MA_command(uint8_t command) {
    uint8_t txBuff[1] = {command};
    SPI_RW(SPI_SLAVE_MMC5983MA, txBuff, NULL, 1);
    ESP_LOGD(TAG, "Command %X sent to MMC5883MA", command);
    return ESP_OK;
}

esp_err_t MMC5983MA_writeSingleByte(uint8_t adress, uint8_t value) {
    uint8_t txBuff[2] = {adress,value};
    SPI_RW(SPI_SLAVE_MMC5983MA, txBuff, NULL, 2);
    ESP_LOGD(TAG, "Byte sent to MMC5883MA Adress: %X Value: %X", txBuff[0], txBuff[1]);
    return ESP_OK;
}

esp_err_t MMC5983MA_init()
{

	uint8_t buf[2] = {0};
	MMC5983MA_read(PROD_ID_REG, buf, 1);

	if(buf[1] == PROD_ID){
		ESP_LOGI(TAG, "MMC5883MA initialization returned: %X", buf[1]);
		return ESP_OK;
	}
	ESP_LOGW(TAG, "WARNING: MMC5883MA initialization returned: %X", buf[1]);
	return ESP_FAIL;
}



esp_err_t MMC5983MA_setRegisterBit(const uint8_t registerAddress, const uint8_t bitMask)
{
    uint8_t value = MMC5983MA_readSingleByte(registerAddress);
    value |= bitMask;
    MMC5983MA_writeSingleByte(registerAddress, value);
    ESP_LOGD(TAG, "MMC5883MA %X Register set %X", registerAddress, value);
    return ESP_OK;
}

esp_err_t MMC5983MA_clearRegisterBit(const uint8_t registerAddress, const uint8_t bitMask)
{
    uint8_t value = MMC5983MA_readSingleByte(registerAddress);
    value &= ~bitMask;
    MMC5983MA_writeSingleByte(registerAddress, value);
    ESP_LOGD(TAG, "MMC5883MA %X Register set %X", registerAddress, value);
    return ESP_OK;
}

bool MMC5983MA_isRegisterSet(const uint8_t registerAddress, const uint8_t bitMask)
{
    uint8_t value = MMC5983MA_readSingleByte(registerAddress);
    return (value & bitMask);
}




void MMC5983MA_setControlBit(uint8_t registerAddress, const uint8_t bitMask)
{
    uint8_t *shadowRegister = NULL;

    // Which register are we referring to?
    switch (registerAddress)
    {
    case INT_CTRL_0_REG:
    {
        shadowRegister = &controlBitMemory.internalControl0;
    }
    break;

    case INT_CTRL_1_REG:
    {
        shadowRegister = &controlBitMemory.internalControl1;
    }
    break;

    case INT_CTRL_2_REG:
    {
        shadowRegister = &controlBitMemory.internalControl2;
    }
    break;

    case INT_CTRL_3_REG:
    {
        shadowRegister = &controlBitMemory.internalControl3;
    }
    break;

    default:
        break;
    }

    if (shadowRegister)
    {
        *shadowRegister |= bitMask;
        MMC5983MA_writeSingleByte(registerAddress, *shadowRegister);
    }
}

void MMC5983MA_clearControlBit(uint8_t registerAddress, const uint8_t bitMask)
{
    uint8_t *shadowRegister = NULL;

    // Which register are we referring to?
    switch (registerAddress)
    {
    case INT_CTRL_0_REG:
    {
        shadowRegister = &controlBitMemory.internalControl0;
    }
    break;

    case INT_CTRL_1_REG:
    {
        shadowRegister = &controlBitMemory.internalControl1;
    }
    break;

    case INT_CTRL_2_REG:
    {
        shadowRegister = &controlBitMemory.internalControl2;
    }
    break;

    case INT_CTRL_3_REG:
    {
        shadowRegister = &controlBitMemory.internalControl3;
    }
    break;

    default:
        break;
    }

    if (shadowRegister)
    {
        *shadowRegister &= ~bitMask;
        MMC5983MA_writeSingleByte(registerAddress, *shadowRegister);
    }
}

bool MMC5983MA_isControlBitSet(uint8_t registerAddress, const uint8_t bitMask)
{
    // Which register are we referring to?
    switch (registerAddress)
    {
    case INT_CTRL_0_REG:
    {
        return (controlBitMemory.internalControl0 & bitMask);
    }
    break;

    case INT_CTRL_1_REG:
    {
        return (controlBitMemory.internalControl1 & bitMask);
    }
    break;

    case INT_CTRL_2_REG:
    {
        return (controlBitMemory.internalControl2 & bitMask);
    }
    break;

    case INT_CTRL_3_REG:
    {
        return (controlBitMemory.internalControl3 & bitMask);
    }
    break;

    default:
        break;
    }

    return false;
}



int MMC5983MA_getTemperature()
{
    // Send command to device. Since TM_T clears itself we don't need to
    // use the shadow register for this - we can send the command directly to the IC.
    MMC5983MA_setRegisterBit(INT_CTRL_0_REG, TM_T);

    // Wait until measurement is completed
    do
    {
        // Wait a little so we won't flood MMC with requests
    	vTaskDelay(5 / portTICK_PERIOD_MS);
    } while (!MMC5983MA_isRegisterSet(STATUS_REG, MEAS_T_DONE));

    // Get raw temperature value from the IC.
    uint8_t result = MMC5983MA_readSingleByte(T_OUT_REG);

    // Convert it using the equation provided in the datasheet
    //float temperature = -75.0f + (static_cast<float>(result) * (200.0f / 255.0f)); TODO Poprawic wzor

    // Return the integer part of the temperature.
    //
    return result; //static_cast<int>(temperature);
}

void MMC5983MA_softReset()
{
    // Since SW_RST bit clears itself we don't need to to through the shadow
    // register for this - we can send the command directly to the IC.
    MMC5983MA_setRegisterBit(INT_CTRL_1_REG, SW_RST);

    // The reset time is 10 msec. but we'll wait 15 msec. just in case.
    vTaskDelay(15 / portTICK_PERIOD_MS);
}

void MMC5983MA_enableInterrupt()
{
    // This bit must be set through the shadow memory or we won't be
    // able to check if interrupts are enabled using isInterruptEnabled()
	MMC5983MA_setControlBit(INT_CTRL_0_REG, INT_MEAS_DONE_EN);
}

void MMC5983MA_disableInterrupt()
{
    // This bit must be cleared through the shadow memory or we won't be
    // able to check if interrupts are enabled using isInterruptEnabled()
	MMC5983MA_clearControlBit(INT_CTRL_0_REG, INT_MEAS_DONE_EN);
}

bool MMC5983MA_isInterruptEnabled()
{
    // Get the bit value from the shadow register since the IC does not
    // allow reading INT_CTRL_0_REG register.
    return MMC5983MA_isControlBitSet(INT_CTRL_0_REG, INT_MEAS_DONE_EN);
}

void MMC5983MA_enable3WireSPI()
{
    // This bit must be set through the shadow memory or we won't be
    // able to check if SPI is enabled using isSPIEnabled()
	MMC5983MA_setControlBit(INT_CTRL_3_REG, SPI_3W);
}

void MMC5983MA_disable3WireSPI()
{
    // This bit must be cleared through the shadow memory or we won't be
    // able to check if is is enabled using isSPIEnabled()
	MMC5983MA_clearControlBit(INT_CTRL_3_REG, SPI_3W);
}

bool MMC5983MA_is3WireSPIEnabled()
{
    // Get the bit value from the shadow register since the IC does not
    // allow reading INT_CTRL_3_REG register.
    return MMC5983MA_isControlBitSet(INT_CTRL_3_REG, SPI_3W);
}

void MMC5983MA_performSetOperation()
{
    // Since SET bit clears itself we don't need to to through the shadow
    // register for this - we can send the command directly to the IC.
    MMC5983MA_setRegisterBit(INT_CTRL_0_REG, SET_OPERATION);

    // Wait until bit clears itself.
    vTaskDelay(1 / portTICK_PERIOD_MS);
}

void MMC5983MA_performResetOperation()
{
    // Since RESET bit clears itself we don't need to to through the shadow
    // register for this - we can send the command directly to the IC.
    MMC5983MA_setRegisterBit(INT_CTRL_0_REG, RESET_OPERATION);

    // Wait until bit clears itself.
    vTaskDelay(1 / portTICK_PERIOD_MS);
}

void MMC5983MA_enableAutomaticSetReset()
{
    // This bit must be set through the shadow memory or we won't be
    // able to check if automatic set/reset is enabled using isAutomaticSetResetEnabled()
	MMC5983MA_setControlBit(INT_CTRL_0_REG, AUTO_SR_EN);
}

void MMC5983MA_disableAutomaticSetReset()
{
    // This bit must be cleared through the shadow memory or we won't be
    // able to check if automatic set/reset is enabled using isAutomaticSetResetEnabled()
	MMC5983MA_clearControlBit(INT_CTRL_0_REG, AUTO_SR_EN);
}

bool MMC5983MA_isAutomaticSetResetEnabled()
{
    // Get the bit value from the shadow register since the IC does not
    // allow reading INT_CTRL_0_REG register.
    return MMC5983MA_isControlBitSet(INT_CTRL_0_REG, AUTO_SR_EN);
}

void MMC5983MA_enableXChannel()
{
    // This bit must be cleared through the shadow memory or we won't be
    // able to check if the channel is enabled using isXChannelEnabled()
    // and since it's a inhibit bit it must be cleared so X channel will
    // be enabled.
	MMC5983MA_clearControlBit(INT_CTRL_1_REG, X_INHIBIT);
}

void MMC5983MA_disableXChannel()
{
    // This bit must be set through the shadow memory or we won't be
    // able to check if the channel is enabled using isXChannelEnabled()
    // and since it's a inhibit bit it must be set so X channel will
    // be disabled.
	MMC5983MA_setControlBit(INT_CTRL_1_REG, X_INHIBIT);
}

bool MMC5983MA_isXChannelEnabled()
{
    // Get the bit value from the shadow register since the IC does not
    // allow reading INT_CTRL_1_REG register.
    return MMC5983MA_isControlBitSet(INT_CTRL_1_REG, X_INHIBIT);
}

void MMC5983MA_enableYZChannels()
{
    // This bit must be cleared through the shadow memory or we won't be
    // able to check if channels are enabled using areYZChannelsEnabled()
    // and since it's a inhibit bit it must be cleared so X channel will
    // be enabled.
	MMC5983MA_clearControlBit(INT_CTRL_1_REG, YZ_INHIBIT);
}

void MMC5983MA_disableYZChannels()
{
    // This bit must be set through the shadow memory or we won't be
    // able to check if channels are enabled using areYZChannelsEnabled()
    // and since it's a inhibit bit it must be cleared so X channel will
    // be disabled.
	MMC5983MA_setControlBit(INT_CTRL_1_REG, YZ_INHIBIT);
}

bool MMC5983MA_areYZChannelsEnabled()
{
    // Get the bit value from the shadow register since the IC does not
    // allow reading INT_CTRL_1_REG register.
    return MMC5983MA_isControlBitSet(INT_CTRL_1_REG, YZ_INHIBIT);
}

void MMC5983MA_setFilterBandwidth(uint16_t bandwidth)
{
    // These must be set/cleared using the shadow memory since it can be read
    // using getFilterBandwith()
    switch (bandwidth)
    {
    case 800:
    {
    	MMC5983MA_setControlBit(INT_CTRL_1_REG, BW0);
    	MMC5983MA_setControlBit(INT_CTRL_1_REG, BW1);
    }
    break;

    case 400:
    {
    	MMC5983MA_clearControlBit(INT_CTRL_1_REG, BW0);
    	MMC5983MA_setControlBit(INT_CTRL_1_REG, BW1);
    }
    break;

    case 200:
    {
    	MMC5983MA_setControlBit(INT_CTRL_1_REG, BW0);
    	MMC5983MA_clearControlBit(INT_CTRL_1_REG, BW1);
    }
    break;

    case 100:
    default:
    {
    	MMC5983MA_clearControlBit(INT_CTRL_1_REG, BW0);
    	MMC5983MA_clearControlBit(INT_CTRL_1_REG, BW1);
    }
    break;
    }
}

uint16_t MMC5983MA_getFilterBandwith()
{
    bool bw0 = MMC5983MA_isControlBitSet(INT_CTRL_1_REG, BW0);
    bool bw1 = MMC5983MA_isControlBitSet(INT_CTRL_1_REG, BW1);

    uint8_t value = (bw1 ? 2 : 0) + (bw0 ? 1 : 0);
    uint16_t retVal = 0;
    switch (value)
    {
    case 1:
        retVal = 200;
        break;

    case 2:
        retVal = 400;
        break;

    case 3:
        retVal = 800;
        break;

    default:
    case 0:
        retVal = 100;
        break;
    }

    return retVal;
}

void MMC5983MA_enableContinuousMode()
{
    // This bit must be set through the shadow memory or we won't be
    // able to check if continuous mode is enabled using isContinuousModeEnabled()
	MMC5983MA_setControlBit(INT_CTRL_2_REG, CMM_EN);
}

void MMC5983MA_disableContinuousMode()
{
    // This bit must be cleared through the shadow memory or we won't be
    // able to check if continuous mode is enabled using isContinuousModeEnabled()
	MMC5983MA_clearControlBit(INT_CTRL_2_REG, CMM_EN);
}

bool MMC5983MA_isContinuousModeEnabled()
{
    // Get the bit value from the shadow register since the IC does not
    // allow reading INT_CTRL_2_REG register.
    return MMC5983MA_isControlBitSet(INT_CTRL_2_REG, CMM_EN);
}

void MMC5983MA_setContinuousModeFrequency(uint16_t frequency)
{
    // These must be set/cleared using the shadow memory since it can be read
    // using getContinuousModeFrequency()
    switch (frequency)
    {
    case 1:
    {
        // CM_FREQ[2:0] = 001
    	MMC5983MA_clearControlBit(INT_CTRL_2_REG, CM_FREQ_2);
        MMC5983MA_clearControlBit(INT_CTRL_2_REG, CM_FREQ_1);
        MMC5983MA_setControlBit(INT_CTRL_2_REG, CM_FREQ_0);
    }
    break;

    case 10:
    {
        // CM_FREQ[2:0] = 010
    	MMC5983MA_clearControlBit(INT_CTRL_2_REG, CM_FREQ_2);
        MMC5983MA_setControlBit(INT_CTRL_2_REG, CM_FREQ_1);
        MMC5983MA_clearControlBit(INT_CTRL_2_REG, CM_FREQ_0);
    }
    break;

    case 20:
    {
        // CM_FREQ[2:0] = 011
    	MMC5983MA_clearControlBit(INT_CTRL_2_REG, CM_FREQ_2);
    	MMC5983MA_setControlBit(INT_CTRL_2_REG, CM_FREQ_1);
    	MMC5983MA_setControlBit(INT_CTRL_2_REG, CM_FREQ_0);
    }
    break;

    case 50:
    {
        // CM_FREQ[2:0] = 100
    	MMC5983MA_setControlBit(INT_CTRL_2_REG, CM_FREQ_2);
    	MMC5983MA_clearControlBit(INT_CTRL_2_REG, CM_FREQ_1);
    	MMC5983MA_clearControlBit(INT_CTRL_2_REG, CM_FREQ_0);
    }
    break;

    case 100:
    {
        // CM_FREQ[2:0] = 101
    	MMC5983MA_setControlBit(INT_CTRL_2_REG, CM_FREQ_2);
    	MMC5983MA_clearControlBit(INT_CTRL_2_REG, CM_FREQ_1);
    	MMC5983MA_setControlBit(INT_CTRL_2_REG, CM_FREQ_0);
    }
    break;

    case 200:
    {
        // CM_FREQ[2:0] = 110
    	MMC5983MA_setControlBit(INT_CTRL_2_REG, CM_FREQ_2);
    	MMC5983MA_setControlBit(INT_CTRL_2_REG, CM_FREQ_1);
    	MMC5983MA_clearControlBit(INT_CTRL_2_REG, CM_FREQ_0);
    }
    break;

    case 1000:
    {
        // CM_FREQ[2:0] = 111
    	MMC5983MA_setControlBit(INT_CTRL_2_REG, CM_FREQ_2);
    	MMC5983MA_setControlBit(INT_CTRL_2_REG, CM_FREQ_1);
    	MMC5983MA_setControlBit(INT_CTRL_2_REG, CM_FREQ_0);
    }
    break;

    case 0:
    default:
    {
        // CM_FREQ[2:0] = 000
    	MMC5983MA_clearControlBit(INT_CTRL_2_REG, CM_FREQ_2);
    	MMC5983MA_clearControlBit(INT_CTRL_2_REG, CM_FREQ_1);
    	MMC5983MA_clearControlBit(INT_CTRL_2_REG, CM_FREQ_0);
    }
    break;
    }
}

uint16_t MMC5983MA_getContinuousModeFrequency()
{
    // Since we cannot read INT_CTRL_2_REG we evaluate the shadow
    // memory contents and return the corresponding frequency.

    // Remove unwanted bits
    uint8_t registerValue = controlBitMemory.internalControl2 & 0x07;
    uint16_t frequency = 0;

    switch (registerValue)
    {
    case 0x01:
    {
        frequency = 1;
    }
    break;

    case 0x02:
    {
        frequency = 10;
    }
    break;

    case 0x03:
    {
        frequency = 20;
    }
    break;

    case 0x04:
    {
        frequency = 50;
    }
    break;

    case 0x05:
    {
        frequency = 100;
    }
    break;

    case 0x06:
    {
        frequency = 200;
    }
    break;

    case 0x07:
    {
        frequency = 1000;
    }
    break;

    case 0:
    default:
        break;
    }

    return frequency;
}

void MMC5983MA_enablePeriodicSet()
{
    // This bit must be set through the shadow memory or we won't be
    // able to check if periodic set is enabled using isContinuousModeEnabled()
	MMC5983MA_setControlBit(INT_CTRL_2_REG, EN_PRD_SET);
}

void MMC5983MA_disablePeriodicSet()
{
    // This bit must be cleared through the shadow memory or we won't be
    // able to check if periodic set is enabled using isContinuousModeEnabled()
	MMC5983MA_clearControlBit(INT_CTRL_2_REG, EN_PRD_SET);
}

bool MMC5983MA_isPeriodicSetEnabled()
{
    // Get the bit value from the shadow register since the IC does not
    // allow reading INT_CTRL_2_REG register.
    return MMC5983MA_isControlBitSet(INT_CTRL_2_REG, EN_PRD_SET);
}

void MMC5983MA_setPeriodicSetSamples(const uint16_t numberOfSamples)
{
    // We must use the shadow memory to do all bits manipulations but
    // we need to access the shadow memory directly, change bits and
    // write back at once.
    switch (numberOfSamples)
    {
    case 25:
    {
        // PRD_SET[2:0] = 001
    	MMC5983MA_clearControlBit(INT_CTRL_2_REG, PRD_SET_2);
    	MMC5983MA_clearControlBit(INT_CTRL_2_REG, PRD_SET_1);
    	MMC5983MA_setControlBit(INT_CTRL_2_REG, PRD_SET_0);
    }
    break;

    case 75:
    {
        // PRD_SET[2:0] = 010
    	MMC5983MA_clearControlBit(INT_CTRL_2_REG, PRD_SET_2);
    	MMC5983MA_setControlBit(INT_CTRL_2_REG, PRD_SET_1);
    	MMC5983MA_clearControlBit(INT_CTRL_2_REG, PRD_SET_0);
    }
    break;

    case 100:
    {
        // PRD_SET[2:0] = 011
    	MMC5983MA_clearControlBit(INT_CTRL_2_REG, PRD_SET_2);
    	MMC5983MA_setControlBit(INT_CTRL_2_REG, PRD_SET_1);
    	MMC5983MA_setControlBit(INT_CTRL_2_REG, PRD_SET_0);
    }
    break;

    case 250:
    {
        // PRD_SET[2:0] = 100
    	MMC5983MA_setControlBit(INT_CTRL_2_REG, PRD_SET_2);
    	MMC5983MA_clearControlBit(INT_CTRL_2_REG, PRD_SET_1);
    	MMC5983MA_clearControlBit(INT_CTRL_2_REG, PRD_SET_0);
    }
    break;

    case 500:
    {
        // PRD_SET[2:0] = 101
    	MMC5983MA_setControlBit(INT_CTRL_2_REG, PRD_SET_2);
    	MMC5983MA_clearControlBit(INT_CTRL_2_REG, PRD_SET_1);
    	MMC5983MA_setControlBit(INT_CTRL_2_REG, PRD_SET_0);
    }
    break;

    case 1000:
    {
        // PRD_SET[2:0] = 110
    	MMC5983MA_setControlBit(INT_CTRL_2_REG, PRD_SET_2);
    	MMC5983MA_setControlBit(INT_CTRL_2_REG, PRD_SET_1);
    	MMC5983MA_clearControlBit(INT_CTRL_2_REG, PRD_SET_0);
    }
    break;

    case 2000:
    {
        // PRD_SET[2:0] = 111
    	MMC5983MA_setControlBit(INT_CTRL_2_REG, PRD_SET_2);
    	MMC5983MA_setControlBit(INT_CTRL_2_REG, PRD_SET_1);
    	MMC5983MA_setControlBit(INT_CTRL_2_REG, PRD_SET_0);
    }
    break;

    case 1:
    default:
    {
        // PRD_SET[2:0] = 000
    	MMC5983MA_clearControlBit(INT_CTRL_2_REG, PRD_SET_2);
    	MMC5983MA_clearControlBit(INT_CTRL_2_REG, PRD_SET_1);
    	MMC5983MA_clearControlBit(INT_CTRL_2_REG, PRD_SET_0);
    }
    break;
    }
}

uint16_t MMC5983MA_getPeriodicSetSamples()
{

    // Since we cannot read INT_CTRL_2_REG we evaluate the shadow
    // memory contents and return the corresponding period.

    // Remove unwanted bits
    uint8_t registerValue = controlBitMemory.internalControl2 & 0x70;
    uint16_t period = 1;

    switch (registerValue)
    {
    case 0x10:
    {
        period = 25;
    }
    break;

    case 0x20:
    {
        period = 75;
    }
    break;

    case 0x30:
    {
        period = 100;
    }
    break;

    case 0x40:
    {
        period = 250;
    }
    break;

    case 0x50:
    {
        period = 500;
    }
    break;

    case 0x60:
    {
        period = 1000;
    }
    break;

    case 0x70:
    {
        period = 2000;
    }
    break;

    case 0x0:
    default:
        break;
    }

    return period;
}

void MMC5983MA_applyExtraCurrentPosToNeg()
{
    // This bit must be set through the shadow memory or we won't be
    // able to check if extra current is applied using isExtraCurrentAppliedPosToNeg()
	MMC5983MA_setControlBit(INT_CTRL_3_REG, ST_ENP);
}

void MMC5983MA_removeExtraCurrentPosToNeg()
{
    // This bit must be cleared through the shadow memory or we won't be
    // able to check if extra current is applied using isExtraCurrentAppliedPosToNeg()
	MMC5983MA_clearControlBit(INT_CTRL_3_REG, ST_ENP);
}

bool MMC5983MA_isExtraCurrentAppliedPosToNeg()
{
    // Get the bit value from the shadow register since the IC does not
    // allow reading INT_CTRL_3_REG register.
    return MMC5983MA_isControlBitSet(INT_CTRL_3_REG, ST_ENP);
}

void MMC5983MA_applyExtracurrentNegToPos()
{
    // This bit must be set through the shadow memory or we won't be
    // able to check if extra current is applied using isExtraCurrentAppliedNegToPos()
	MMC5983MA_setControlBit(INT_CTRL_3_REG, ST_ENM);
}

void MMC5983MA_removeExtracurrentNegToPos()
{
    // This bit must be cleared through the shadow memory or we won't be
    // able to check if extra current is applied using isExtraCurrentAppliedNegToPos()
	MMC5983MA_clearControlBit(INT_CTRL_3_REG, ST_ENM);
}

bool MMC5983MA_isExtraCurrentAppliedNegToPos()
{
    // Get the bit value from the shadow register since the IC does not
    // allow reading INT_CTRL_3_REG register.
    return MMC5983MA_isControlBitSet(INT_CTRL_3_REG, ST_ENM);
}
/*
uint32_t MMC5983MA_getMeasurementX()
{
    // Send command to device. TM_M self clears so we can access it directly.
    MMC5983MA_setRegisterBit(INT_CTRL_0_REG, TM_M);

    // Wait until measurement is completed
    do
    {
        // Wait a little so we won't flood MMC with requests
        delay(5);
    } while (!MMC5983MA_isRegisterSet(STATUS_REG, MEAS_M_DONE));

    uint32_t temp = 0;
    uint32_t result = 0;
    uint8_t buffer[7] = {0};

    MMC5983MA_read(X_OUT_0_REG, buffer, 7);

    temp = static_cast<uint32_t>(buffer[X_OUT_0_REG]);
    temp = temp << XYZ_0_SHIFT;
    result |= temp;

    temp = static_cast<uint32_t>(buffer[X_OUT_1_REG]);
    temp = temp << XYZ_1_SHIFT;
    result |= temp;

    temp = static_cast<uint32_t>(buffer[XYZ_OUT_2_REG]);
    temp &= X2_MASK;
    temp = temp >> 6;
    result |= temp;
    return result;
}

uint32_t MMC5983MA_getMeasurementY()
{
    // Send command to device. TM_M self clears so we can access it directly.
    MMC5983MA_setRegisterBit(INT_CTRL_0_REG, TM_M);

    // Wait until measurement is completed
    do
    {
        // Wait a little so we won't flood MMC with requests
        delay(5);
    } while (!MMC5983MA_isRegisterSet(STATUS_REG, MEAS_M_DONE));

    uint32_t temp = 0;
    uint32_t result = 0;
    uint8_t registerValue = 0;

    registerValue = (MMC5983MA_readSingleByte(Y_OUT_0_REG));

    temp = static_cast<uint32_t>(registerValue);
    temp = temp << XYZ_0_SHIFT;
    result |= temp;

    registerValue = (MMC5983MA_readSingleByte(Y_OUT_1_REG));

    temp = static_cast<uint32_t>(registerValue);
    temp = temp << XYZ_1_SHIFT;
    result |= temp;

    registerValue = (MMC5983MA_readSingleByte(XYZ_OUT_2_REG));
    temp = static_cast<uint32_t>(registerValue);
    temp &= Y2_MASK;
    temp = temp >> 4;
    result |= temp;
    return result;
}

uint32_t MMC5983MA_getMeasurementZ()
{
    // Send command to device. TM_M self clears so we can access it directly.
    MMC5983MA_setRegisterBit(INT_CTRL_0_REG, TM_M);

    // Wait until measurement is completed
    do
    {
        // Wait a little so we won't flood MMC with requests
        delay(5);
    } while (!MMC5983MA_isRegisterSet(STATUS_REG, MEAS_M_DONE));

    uint32_t temp = 0;
    uint32_t result = 0;
    uint8_t registerValue = 0;

    registerValue = (MMC5983MA_readSingleByte(Z_OUT_0_REG));

    temp = static_cast<uint32_t>(registerValue);
    temp = temp << XYZ_0_SHIFT;
    result |= temp;

    registerValue = (MMC5983MA_readSingleByte(Z_OUT_1_REG));

    temp = static_cast<uint32_t>(registerValue);
    temp = temp << XYZ_1_SHIFT;
    result |= temp;

    registerValue = (MMC5983MA_readSingleByte(XYZ_OUT_2_REG));

    temp = static_cast<uint32_t>(registerValue);
    temp &= Z2_MASK;
    temp = temp >> 2;
    result |= temp;
    return result;
}

*/

