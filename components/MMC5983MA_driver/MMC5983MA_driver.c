#include <stdio.h>
#include "esp_err.h"
#include "esp_check.h"
#include "esp_log.h"
#include "MMC5983MA_driver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "BOARD.h"
#include <string.h>

static const char* TAG = "MMC5983MA";
MMC5983MA_t MMC5983MA_d;

#if !defined SPI_SLAVE_MMC5983MA_PIN
esp_err_t MMC5983MA_init() {return ESP_OK;}
esp_err_t MMC5983MA_readMeas() {return ESP_OK;}
esp_err_t MMC5983MA_getMeas(MMC5983MA_meas_t * meas) {return ESP_OK;}

#else
#define READ_REG(x) (0x80 | x)
static esp_err_t MMC5983MA_read(uint8_t addr, uint8_t * data_in, uint16_t length) __attribute__((unused));
static uint8_t MMC5983MA_readSingleByte(uint8_t  address) __attribute__((unused));
static esp_err_t MMC5983MA_writeSingleByte(uint8_t adress, uint8_t value) __attribute__((unused));
static esp_err_t MMC5983MA_setRegisterBit(uint8_t registerAddress, uint8_t bitMask) __attribute__((unused));
static esp_err_t MMC5983MA_clearRegisterBit(const uint8_t registerAddress, const uint8_t bitMask) __attribute__((unused));
static bool MMC5983MA_isRegisterSet(const uint8_t registerAddress, const uint8_t bitMask) __attribute__((unused));
static void MMC5983MA_setControlBit(uint8_t registerAddress, const uint8_t bitMask) __attribute__((unused));
static void MMC5983MA_clearControlBit(uint8_t registerAddress, const uint8_t bitMask) __attribute__((unused));
static bool MMC5983MA_isControlBitSet(uint8_t registerAddress, const uint8_t bitMask) __attribute__((unused));
static int MMC5983MA_getTemperature() __attribute__((unused));
static void MMC5983MA_softReset() __attribute__((unused));
static void MMC5983MA_enableInterrupt() __attribute__((unused));
static void MMC5983MA_disableInterrupt() __attribute__((unused));
static bool MMC5983MA_isInterruptEnabled() __attribute__((unused));
static void MMC5983MA_enable3WireSPI() __attribute__((unused));
static void MMC5983MA_disable3WireSPI() __attribute__((unused));
static bool MMC5983MA_is3WireSPIEnabled() __attribute__((unused));
static void MMC5983MA_performSetOperation() __attribute__((unused));
static void MMC5983MA_performResetOperation() __attribute__((unused));
static void MMC5983MA_enableAutomaticSetReset() __attribute__((unused));
static void MMC5983MA_disableAutomaticSetReset() __attribute__((unused));
static bool MMC5983MA_isAutomaticSetResetEnabled() __attribute__((unused));
static void MMC5983MA_enableXChannel() __attribute__((unused));
static void MMC5983MA_disableXChannel() __attribute__((unused));
static bool MMC5983MA_isXChannelEnabled() __attribute__((unused));
static void MMC5983MA_enableYZChannels() __attribute__((unused));
static void MMC5983MA_disableYZChannels() __attribute__((unused));
static bool MMC5983MA_areYZChannelsEnabled() __attribute__((unused));
static void MMC5983MA_setFilterBandwidth(mmc5983ma_band_t bandwidth) __attribute__((unused));
static uint16_t MMC5983MA_getFilterBandwith() __attribute__((unused));
static void MMC5983MA_enableContinuousMode() __attribute__((unused));
static void MMC5983MA_disableContinuousMode() __attribute__((unused));
static bool MMC5983MA_isContinuousModeEnabled() __attribute__((unused));
static void MMC5983MA_setContinuousModeFrequency(mmc5983ma_cm_freq_t frequency) __attribute__((unused));
static uint16_t MMC5983MA_getContinuousModeFrequency() __attribute__((unused));
static void MMC5983MA_enablePeriodicSet() __attribute__((unused));
static void MMC5983MA_disablePeriodicSet() __attribute__((unused));
static bool MMC5983MA_isPeriodicSetEnabled() __attribute__((unused));
static void MMC5983MA_setPeriodicSetSamples(const uint16_t numberOfSamples) __attribute__((unused));
static uint16_t MMC5983MA_getPeriodicSetSamples() __attribute__((unused));
static void MMC5983MA_applyExtraCurrentPosToNeg() __attribute__((unused));
static void MMC5983MA_removeExtraCurrentPosToNeg() __attribute__((unused));
static bool MMC5983MA_isExtraCurrentAppliedPosToNeg() __attribute__((unused));
static void MMC5983MA_applyExtracurrentNegToPos() __attribute__((unused));
static void MMC5983MA_removeExtracurrentNegToPos() __attribute__((unused));
static bool MMC5983MA_isExtraCurrentAppliedNegToPos() __attribute__((unused));
static uint32_t MMC5983MA_getMeasurementX() __attribute__((unused));
static uint32_t MMC5983MA_getMeasurementY() __attribute__((unused));
static uint32_t MMC5983MA_getMeasurementZ() __attribute__((unused));

static controlBitMemory_t controlBitMemory;
static spi_dev_handle_t spi_dev_handle_MMC5983MA;

esp_err_t MMC5983MA_spi_init(void)
{
	if(SPI_checkInit() != ESP_OK){
		ESP_LOGE(TAG, "SPI controller not initialized! Use SPI_init() in main.c");
		return ESP_ERR_INVALID_STATE;
	}

	/* CONFIGURE SPI DEVICE */
	/* Max SCK frequency - 10MHz */
	ESP_RETURN_ON_ERROR(SPI_registerDevice(&spi_dev_handle_MMC5983MA, SPI_SLAVE_MMC5983MA_PIN,
											SPI_SCK_10MHZ, 1, 2, 6), TAG, "SPI register failed");

	return ESP_OK;
}


esp_err_t MMC5983MA_init()
{
	MMC5983MA_spi_init();
	// Reset !!!
	MMC5983MA_enableAutomaticSetReset();
	MMC5983MA_enablePeriodicSet();
	MMC5983MA_setPeriodicSetSamples(100);
	MMC5983MA_setFilterBandwidth(MMC5983MA_BAND_100);
	MMC5983MA_setContinuousModeFrequency(MMC5983MA_FREQ_100HZ);
	MMC5983MA_enableContinuousMode();

	MMC5983MA_d.offsetX =   1741.0f;
	MMC5983MA_d.offsetY =  11276.5f;
	MMC5983MA_d.offsetZ =  -9830.4f;

	MMC5983MA_d.gainX = 6839.0f;
	MMC5983MA_d.gainY = 6660.5f;
	MMC5983MA_d.gainZ = 6553.6f;

	uint8_t ID = 0;
	MMC5983MA_read(MMC_PROD_ID_REG, &ID, 1);

	if(ID == MMC_PROD_ID){
		ESP_LOGI(TAG, "MMC5883MA initialization returned: 0x%X", ID);
		return ESP_OK;
	}
	ESP_LOGW(TAG, "WARNING: MMC5883MA initialization returned: 0x%X", ID);
	return ESP_FAIL;
}

esp_err_t MMC5983MA_readMeas()
{
	if(MMC5983MA_isRegisterSet(MMC_STATUS_REG, MMC_MEAS_M_DONE)){
		uint8_t buffer[7] = {0};
		MMC5983MA_read(MMC_X_OUT_0_REG, buffer, 7);

		int32_t Xraw = (((uint32_t) buffer[0]) << 10) | (((uint32_t) buffer[1]) << 2) | ((buffer[6] & 0xC0) >> 6);
		int32_t Yraw = (((uint32_t) buffer[2]) << 10) | (((uint32_t) buffer[3]) << 2) | ((buffer[6] & 0x30) >> 4);
		int32_t Zraw = (((uint32_t) buffer[4]) << 10) | (((uint32_t) buffer[5]) << 2) | ((buffer[6] & 0x0C) >> 2);



		MMC5983MA_d.Xraw = Xraw;
		MMC5983MA_d.Yraw = Yraw;
		MMC5983MA_d.Zraw = Zraw;

		MMC5983MA_d.meas.magX = ((float)((Xraw - 131072)*2)) / 16384;
		MMC5983MA_d.meas.magY = ((float)((Yraw - 131072)*2)) / 16384;
		MMC5983MA_d.meas.magZ = ((float)((Zraw - 131072)*2)) / 16384;




		return ESP_OK;
	}
	return ESP_ERR_NOT_FOUND;
}

esp_err_t MMC5983MA_getMeas(MMC5983MA_meas_t * meas){
	*meas = MMC5983MA_d.meas;

	return ESP_OK;
}

static esp_err_t MMC5983MA_read(uint8_t addr, uint8_t * data_in, uint16_t length) {
	return SPI_transfer(spi_dev_handle_MMC5983MA, 0x02, addr, NULL, data_in, length);
}

static uint8_t MMC5983MA_readSingleByte(uint8_t  addr) {
	uint8_t buf = 0;
	SPI_transfer(spi_dev_handle_MMC5983MA, 0x02, addr, NULL, &buf, 1);

	return buf;
}

static esp_err_t MMC5983MA_writeSingleByte(uint8_t addr, uint8_t data_out) {
	return SPI_transfer(spi_dev_handle_MMC5983MA, 0, addr, &data_out, NULL, 1);
}

static esp_err_t MMC5983MA_setRegisterBit(const uint8_t registerAddress, const uint8_t bitMask)
{
    uint8_t value = MMC5983MA_readSingleByte(registerAddress);
    value |= bitMask;
    MMC5983MA_writeSingleByte(registerAddress, value);
    ESP_LOGD(TAG, "MMC5883MA %X Register set %X", registerAddress, value);
    return ESP_OK;
}

static esp_err_t MMC5983MA_clearRegisterBit(const uint8_t registerAddress, const uint8_t bitMask)
{
    uint8_t value = MMC5983MA_readSingleByte(registerAddress);
    value &= ~bitMask;
    MMC5983MA_writeSingleByte(registerAddress, value);
    ESP_LOGD(TAG, "MMC5883MA %X Register set %X", registerAddress, value);
    return ESP_OK;
}

static bool MMC5983MA_isRegisterSet(const uint8_t registerAddress, const uint8_t bitMask)
{
    uint8_t value = MMC5983MA_readSingleByte(registerAddress);
    return (value & bitMask);
}




static void MMC5983MA_setControlBit(uint8_t registerAddress, const uint8_t bitMask)
{
    uint8_t *shadowRegister = NULL;

    // Which register are we referring to?
    switch (registerAddress)
    {
    case MMC_INT_CTRL_0_REG:
    {
        shadowRegister = &controlBitMemory.internalControl0;
    }
    break;

    case MMC_INT_CTRL_1_REG:
    {
        shadowRegister = &controlBitMemory.internalControl1;
    }
    break;

    case MMC_INT_CTRL_2_REG:
    {
        shadowRegister = &controlBitMemory.internalControl2;
    }
    break;

    case MMC_INT_CTRL_3_REG:
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

static void MMC5983MA_clearControlBit(uint8_t registerAddress, const uint8_t bitMask)
{
    uint8_t *shadowRegister = NULL;

    // Which register are we referring to?
    switch (registerAddress)
    {
    case MMC_INT_CTRL_0_REG:
    {
        shadowRegister = &controlBitMemory.internalControl0;
    }
    break;

    case MMC_INT_CTRL_1_REG:
    {
        shadowRegister = &controlBitMemory.internalControl1;
    }
    break;

    case MMC_INT_CTRL_2_REG:
    {
        shadowRegister = &controlBitMemory.internalControl2;
    }
    break;

    case MMC_INT_CTRL_3_REG:
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

static bool MMC5983MA_isControlBitSet(uint8_t registerAddress, const uint8_t bitMask)
{
    // Which register are we referring to?
    switch (registerAddress)
    {
    case MMC_INT_CTRL_0_REG:
    {
        return (controlBitMemory.internalControl0 & bitMask);
    }
    break;

    case MMC_INT_CTRL_1_REG:
    {
        return (controlBitMemory.internalControl1 & bitMask);
    }
    break;

    case MMC_INT_CTRL_2_REG:
    {
        return (controlBitMemory.internalControl2 & bitMask);
    }
    break;

    case MMC_INT_CTRL_3_REG:
    {
        return (controlBitMemory.internalControl3 & bitMask);
    }
    break;

    default:
        break;
    }

    return false;
}



static int MMC5983MA_getTemperature()
{
    // Send command to device. Since TM_T clears itself we don't need to
    // use the shadow register for this - we can send the command directly to the IC.
    MMC5983MA_setRegisterBit(MMC_INT_CTRL_0_REG, MMC_TM_T);

    // Wait until measurement is completed
    do
    {
        // Wait a little so we won't flood MMC with requests
    	vTaskDelay(5 / portTICK_PERIOD_MS);
    } while (!MMC5983MA_isRegisterSet(MMC_STATUS_REG, MMC_MEAS_T_DONE));

    // Get raw temperature value from the IC.
    uint8_t result = MMC5983MA_readSingleByte(MMC_T_OUT_REG);

    // Convert it using the equation provided in the datasheet
    //float temperature = -75.0f + (static_cast<float>(result) * (200.0f / 255.0f)); TODO Poprawic wzor

    // Return the integer part of the temperature.
    //
    return result; //static_cast<int>(temperature);
}

static void MMC5983MA_softReset()
{
    // Since SW_RST bit clears itself we don't need to to through the shadow
    // register for this - we can send the command directly to the IC.
    MMC5983MA_setRegisterBit(MMC_INT_CTRL_1_REG, MMC_SW_RST);

    // The reset time is 10 msec. but we'll wait 15 msec. just in case.
    vTaskDelay(15 / portTICK_PERIOD_MS);
}

static void MMC5983MA_enableInterrupt()
{
    // This bit must be set through the shadow memory or we won't be
    // able to check if interrupts are enabled using isInterruptEnabled()
	MMC5983MA_setControlBit(MMC_INT_CTRL_0_REG, MMC_INT_MEAS_DONE_EN);
}

static void MMC5983MA_disableInterrupt()
{
    // This bit must be cleared through the shadow memory or we won't be
    // able to check if interrupts are enabled using isInterruptEnabled()
	MMC5983MA_clearControlBit(MMC_INT_CTRL_0_REG, MMC_INT_MEAS_DONE_EN);
}

static bool MMC5983MA_isInterruptEnabled()
{
    // Get the bit value from the shadow register since the IC does not
    // allow reading INT_CTRL_0_REG register.
    return MMC5983MA_isControlBitSet(MMC_INT_CTRL_0_REG, MMC_INT_MEAS_DONE_EN);
}

static void MMC5983MA_enable3WireSPI()
{
    // This bit must be set through the shadow memory or we won't be
    // able to check if SPI is enabled using isSPIEnabled()
	MMC5983MA_setControlBit(MMC_INT_CTRL_3_REG, MMC_SPI_3W);
}

static void MMC5983MA_disable3WireSPI()
{
    // This bit must be cleared through the shadow memory or we won't be
    // able to check if is is enabled using isSPIEnabled()
	MMC5983MA_clearControlBit(MMC_INT_CTRL_3_REG, MMC_SPI_3W);
}

static bool MMC5983MA_is3WireSPIEnabled()
{
    // Get the bit value from the shadow register since the IC does not
    // allow reading INT_CTRL_3_REG register.
    return MMC5983MA_isControlBitSet(MMC_INT_CTRL_3_REG, MMC_SPI_3W);
}

static void MMC5983MA_performSetOperation()
{
    // Since SET bit clears itself we don't need to to through the shadow
    // register for this - we can send the command directly to the IC.
    MMC5983MA_setRegisterBit(MMC_INT_CTRL_0_REG, MMC_SET_OPERATION);

    // Wait until bit clears itself.
    vTaskDelay(1 / portTICK_PERIOD_MS);
}

static void MMC5983MA_performResetOperation()
{
    // Since RESET bit clears itself we don't need to to through the shadow
    // register for this - we can send the command directly to the IC.
    MMC5983MA_setRegisterBit(MMC_INT_CTRL_0_REG, MMC_RESET_OPERATION);

    // Wait until bit clears itself.
    vTaskDelay(1 / portTICK_PERIOD_MS);
}

static void MMC5983MA_enableAutomaticSetReset()
{
    // This bit must be set through the shadow memory or we won't be
    // able to check if automatic set/reset is enabled using isAutomaticSetResetEnabled()
	MMC5983MA_setControlBit(MMC_INT_CTRL_0_REG, MMC_AUTO_SR_EN);
}

static void MMC5983MA_disableAutomaticSetReset()
{
    // This bit must be cleared through the shadow memory or we won't be
    // able to check if automatic set/reset is enabled using isAutomaticSetResetEnabled()
	MMC5983MA_clearControlBit(MMC_INT_CTRL_0_REG, MMC_AUTO_SR_EN);
}

static bool MMC5983MA_isAutomaticSetResetEnabled()
{
    // Get the bit value from the shadow register since the IC does not
    // allow reading INT_CTRL_0_REG register.
    return MMC5983MA_isControlBitSet(MMC_INT_CTRL_0_REG, MMC_AUTO_SR_EN);
}

static void MMC5983MA_enableXChannel()
{
    // This bit must be cleared through the shadow memory or we won't be
    // able to check if the channel is enabled using isXChannelEnabled()
    // and since it's a inhibit bit it must be cleared so X channel will
    // be enabled.
	MMC5983MA_clearControlBit(MMC_INT_CTRL_1_REG, MMC_X_INHIBIT);
}

static void MMC5983MA_disableXChannel()
{
    // This bit must be set through the shadow memory or we won't be
    // able to check if the channel is enabled using isXChannelEnabled()
    // and since it's a inhibit bit it must be set so X channel will
    // be disabled.
	MMC5983MA_setControlBit(MMC_INT_CTRL_1_REG, MMC_X_INHIBIT);
}

static bool MMC5983MA_isXChannelEnabled()
{
    // Get the bit value from the shadow register since the IC does not
    // allow reading INT_CTRL_1_REG register.
    return MMC5983MA_isControlBitSet(MMC_INT_CTRL_1_REG, MMC_X_INHIBIT);
}

static void MMC5983MA_enableYZChannels()
{
    // This bit must be cleared through the shadow memory or we won't be
    // able to check if channels are enabled using areYZChannelsEnabled()
    // and since it's a inhibit bit it must be cleared so X channel will
    // be enabled.
	MMC5983MA_clearControlBit(MMC_INT_CTRL_1_REG, MMC_YZ_INHIBIT);
}

static void MMC5983MA_disableYZChannels()
{
    // This bit must be set through the shadow memory or we won't be
    // able to check if channels are enabled using areYZChannelsEnabled()
    // and since it's a inhibit bit it must be cleared so X channel will
    // be disabled.
	MMC5983MA_setControlBit(MMC_INT_CTRL_1_REG, MMC_YZ_INHIBIT);
}

static bool MMC5983MA_areYZChannelsEnabled()
{
    // Get the bit value from the shadow register since the IC does not
    // allow reading INT_CTRL_1_REG register.
    return MMC5983MA_isControlBitSet(MMC_INT_CTRL_1_REG, MMC_YZ_INHIBIT);
}

static void MMC5983MA_setFilterBandwidth(mmc5983ma_band_t bandwidth)
{
    // These must be set/cleared using the shadow memory since it can be read
    // using getFilterBandwith()
    switch (bandwidth)
    {
    case 800:
    {
    	MMC5983MA_setControlBit(MMC_INT_CTRL_1_REG, MMC_BW0);
    	MMC5983MA_setControlBit(MMC_INT_CTRL_1_REG, MMC_BW1);
    }
    break;

    case 400:
    {
    	MMC5983MA_clearControlBit(MMC_INT_CTRL_1_REG, MMC_BW0);
    	MMC5983MA_setControlBit  (MMC_INT_CTRL_1_REG, MMC_BW1);
    }
    break;

    case 200:
    {
    	MMC5983MA_setControlBit  (MMC_INT_CTRL_1_REG, MMC_BW0);
    	MMC5983MA_clearControlBit(MMC_INT_CTRL_1_REG, MMC_BW1);
    }
    break;

    case 100:
    default:
    {
    	MMC5983MA_clearControlBit(MMC_INT_CTRL_1_REG, MMC_BW0);
    	MMC5983MA_clearControlBit(MMC_INT_CTRL_1_REG, MMC_BW1);
    }
    break;
    }
}

static uint16_t MMC5983MA_getFilterBandwith()
{
    bool bw0 = MMC5983MA_isControlBitSet(MMC_INT_CTRL_1_REG, MMC_BW0);
    bool bw1 = MMC5983MA_isControlBitSet(MMC_INT_CTRL_1_REG, MMC_BW1);

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

static void MMC5983MA_enableContinuousMode()
{
    // This bit must be set through the shadow memory or we won't be
    // able to check if continuous mode is enabled using isContinuousModeEnabled()
	MMC5983MA_setControlBit(MMC_INT_CTRL_2_REG, MMC_CMM_EN);
}

static void MMC5983MA_disableContinuousMode()
{
    // This bit must be cleared through the shadow memory or we won't be
    // able to check if continuous mode is enabled using isContinuousModeEnabled()
	MMC5983MA_clearControlBit(MMC_INT_CTRL_2_REG, MMC_CMM_EN);
}

static bool MMC5983MA_isContinuousModeEnabled()
{
    // Get the bit value from the shadow register since the IC does not
    // allow reading INT_CTRL_2_REG register.
    return MMC5983MA_isControlBitSet(MMC_INT_CTRL_2_REG, MMC_CMM_EN);
}

static void MMC5983MA_setContinuousModeFrequency(mmc5983ma_cm_freq_t frequency)
{
    // These must be set/cleared using the shadow memory since it can be read
    // using getContinuousModeFrequency()
    switch (frequency)
    {
    case 1:
    {
        // CM_FREQ[2:0] = 001
    	MMC5983MA_clearControlBit(MMC_INT_CTRL_2_REG, MMC_CM_FREQ_2);
        MMC5983MA_clearControlBit(MMC_INT_CTRL_2_REG, MMC_CM_FREQ_1);
        MMC5983MA_setControlBit  (MMC_INT_CTRL_2_REG, MMC_CM_FREQ_0);
    }
    break;

    case 10:
    {
        // CM_FREQ[2:0] = 010
    	MMC5983MA_clearControlBit(MMC_INT_CTRL_2_REG, MMC_CM_FREQ_2);
        MMC5983MA_setControlBit  (MMC_INT_CTRL_2_REG, MMC_CM_FREQ_1);
        MMC5983MA_clearControlBit(MMC_INT_CTRL_2_REG, MMC_CM_FREQ_0);
    }
    break;

    case 20:
    {
        // CM_FREQ[2:0] = 011
    	MMC5983MA_clearControlBit(MMC_INT_CTRL_2_REG, MMC_CM_FREQ_2);
    	MMC5983MA_setControlBit  (MMC_INT_CTRL_2_REG, MMC_CM_FREQ_1);
    	MMC5983MA_setControlBit  (MMC_INT_CTRL_2_REG, MMC_CM_FREQ_0);
    }
    break;

    case 50:
    {
        // CM_FREQ[2:0] = 100
    	MMC5983MA_setControlBit  (MMC_INT_CTRL_2_REG, MMC_CM_FREQ_2);
    	MMC5983MA_clearControlBit(MMC_INT_CTRL_2_REG, MMC_CM_FREQ_1);
    	MMC5983MA_clearControlBit(MMC_INT_CTRL_2_REG, MMC_CM_FREQ_0);
    }
    break;

    case 100:
    {
        // CM_FREQ[2:0] = 101
    	MMC5983MA_setControlBit  (MMC_INT_CTRL_2_REG, MMC_CM_FREQ_2);
    	MMC5983MA_clearControlBit(MMC_INT_CTRL_2_REG, MMC_CM_FREQ_1);
    	MMC5983MA_setControlBit  (MMC_INT_CTRL_2_REG, MMC_CM_FREQ_0);
    }
    break;

    case 200:
    {
        // CM_FREQ[2:0] = 110
    	MMC5983MA_setControlBit  (MMC_INT_CTRL_2_REG, MMC_CM_FREQ_2);
    	MMC5983MA_setControlBit  (MMC_INT_CTRL_2_REG, MMC_CM_FREQ_1);
    	MMC5983MA_clearControlBit(MMC_INT_CTRL_2_REG, MMC_CM_FREQ_0);
    }
    break;

    case 1000:
    {
        // CM_FREQ[2:0] = 111
    	MMC5983MA_setControlBit(MMC_INT_CTRL_2_REG, MMC_CM_FREQ_2);
    	MMC5983MA_setControlBit(MMC_INT_CTRL_2_REG, MMC_CM_FREQ_1);
    	MMC5983MA_setControlBit(MMC_INT_CTRL_2_REG, MMC_CM_FREQ_0);
    }
    break;

    case 0:
    default:
    {
        // CM_FREQ[2:0] = 000
    	MMC5983MA_clearControlBit(MMC_INT_CTRL_2_REG, MMC_CM_FREQ_2);
    	MMC5983MA_clearControlBit(MMC_INT_CTRL_2_REG, MMC_CM_FREQ_1);
    	MMC5983MA_clearControlBit(MMC_INT_CTRL_2_REG, MMC_CM_FREQ_0);
    }
    break;
    }
}

static uint16_t MMC5983MA_getContinuousModeFrequency()
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

static void MMC5983MA_enablePeriodicSet()
{
    // This bit must be set through the shadow memory or we won't be
    // able to check if periodic set is enabled using isContinuousModeEnabled()
	MMC5983MA_setControlBit(MMC_INT_CTRL_2_REG, MMC_EN_PRD_SET);
}

static void MMC5983MA_disablePeriodicSet()
{
    // This bit must be cleared through the shadow memory or we won't be
    // able to check if periodic set is enabled using isContinuousModeEnabled()
	MMC5983MA_clearControlBit(MMC_INT_CTRL_2_REG, MMC_EN_PRD_SET);
}

static bool MMC5983MA_isPeriodicSetEnabled()
{
    // Get the bit value from the shadow register since the IC does not
    // allow reading INT_CTRL_2_REG register.
    return MMC5983MA_isControlBitSet(MMC_INT_CTRL_2_REG, MMC_EN_PRD_SET);
}

static void MMC5983MA_setPeriodicSetSamples(const uint16_t numberOfSamples)
{
    // We must use the shadow memory to do all bits manipulations but
    // we need to access the shadow memory directly, change bits and
    // write back at once.
    switch (numberOfSamples)
    {
    case 25:
    {
        // PRD_SET[2:0] = 001
    	MMC5983MA_clearControlBit(MMC_INT_CTRL_2_REG, MMC_PRD_SET_2);
    	MMC5983MA_clearControlBit(MMC_INT_CTRL_2_REG, MMC_PRD_SET_1);
    	MMC5983MA_setControlBit  (MMC_INT_CTRL_2_REG, MMC_PRD_SET_0);
    }
    break;

    case 75:
    {
        // PRD_SET[2:0] = 010
    	MMC5983MA_clearControlBit(MMC_INT_CTRL_2_REG, MMC_PRD_SET_2);
    	MMC5983MA_setControlBit  (MMC_INT_CTRL_2_REG, MMC_PRD_SET_1);
    	MMC5983MA_clearControlBit(MMC_INT_CTRL_2_REG, MMC_PRD_SET_0);
    }
    break;

    case 100:
    {
        // PRD_SET[2:0] = 011
    	MMC5983MA_clearControlBit(MMC_INT_CTRL_2_REG, MMC_PRD_SET_2);
    	MMC5983MA_setControlBit  (MMC_INT_CTRL_2_REG, MMC_PRD_SET_1);
    	MMC5983MA_setControlBit  (MMC_INT_CTRL_2_REG, MMC_PRD_SET_0);
    }
    break;

    case 250:
    {
        // PRD_SET[2:0] = 100
    	MMC5983MA_setControlBit  (MMC_INT_CTRL_2_REG, MMC_PRD_SET_2);
    	MMC5983MA_clearControlBit(MMC_INT_CTRL_2_REG, MMC_PRD_SET_1);
    	MMC5983MA_clearControlBit(MMC_INT_CTRL_2_REG, MMC_PRD_SET_0);
    }
    break;

    case 500:
    {
        // PRD_SET[2:0] = 101
    	MMC5983MA_setControlBit  (MMC_INT_CTRL_2_REG, MMC_PRD_SET_2);
    	MMC5983MA_clearControlBit(MMC_INT_CTRL_2_REG, MMC_PRD_SET_1);
    	MMC5983MA_setControlBit  (MMC_INT_CTRL_2_REG, MMC_PRD_SET_0);
    }
    break;

    case 1000:
    {
        // PRD_SET[2:0] = 110
    	MMC5983MA_setControlBit  (MMC_INT_CTRL_2_REG, MMC_PRD_SET_2);
    	MMC5983MA_setControlBit  (MMC_INT_CTRL_2_REG, MMC_PRD_SET_1);
    	MMC5983MA_clearControlBit(MMC_INT_CTRL_2_REG, MMC_PRD_SET_0);
    }
    break;

    case 2000:
    {
        // PRD_SET[2:0] = 111
    	MMC5983MA_setControlBit(MMC_INT_CTRL_2_REG, MMC_PRD_SET_2);
    	MMC5983MA_setControlBit(MMC_INT_CTRL_2_REG, MMC_PRD_SET_1);
    	MMC5983MA_setControlBit(MMC_INT_CTRL_2_REG, MMC_PRD_SET_0);
    }
    break;

    case 1:
    default:
    {
        // PRD_SET[2:0] = 000
    	MMC5983MA_clearControlBit(MMC_INT_CTRL_2_REG, MMC_PRD_SET_2);
    	MMC5983MA_clearControlBit(MMC_INT_CTRL_2_REG, MMC_PRD_SET_1);
    	MMC5983MA_clearControlBit(MMC_INT_CTRL_2_REG, MMC_PRD_SET_0);
    }
    break;
    }
}

static uint16_t MMC5983MA_getPeriodicSetSamples()
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

static void MMC5983MA_applyExtraCurrentPosToNeg()
{
    // This bit must be set through the shadow memory or we won't be
    // able to check if extra current is applied using isExtraCurrentAppliedPosToNeg()
	MMC5983MA_setControlBit(MMC_INT_CTRL_3_REG, MMC_ST_ENP);
}

static void MMC5983MA_removeExtraCurrentPosToNeg()
{
    // This bit must be cleared through the shadow memory or we won't be
    // able to check if extra current is applied using isExtraCurrentAppliedPosToNeg()
	MMC5983MA_clearControlBit(MMC_INT_CTRL_3_REG, MMC_ST_ENP);
}

static bool MMC5983MA_isExtraCurrentAppliedPosToNeg()
{
    // Get the bit value from the shadow register since the IC does not
    // allow reading INT_CTRL_3_REG register.
    return MMC5983MA_isControlBitSet(MMC_INT_CTRL_3_REG, MMC_ST_ENP);
}

static void MMC5983MA_applyExtracurrentNegToPos()
{
    // This bit must be set through the shadow memory or we won't be
    // able to check if extra current is applied using isExtraCurrentAppliedNegToPos()
	MMC5983MA_setControlBit(MMC_INT_CTRL_3_REG, MMC_ST_ENM);
}

static void MMC5983MA_removeExtracurrentNegToPos()
{
    // This bit must be cleared through the shadow memory or we won't be
    // able to check if extra current is applied using isExtraCurrentAppliedNegToPos()
	MMC5983MA_clearControlBit(MMC_INT_CTRL_3_REG, MMC_ST_ENM);
}

static bool MMC5983MA_isExtraCurrentAppliedNegToPos()
{
    // Get the bit value from the shadow register since the IC does not
    // allow reading INT_CTRL_3_REG register.
    return MMC5983MA_isControlBitSet(MMC_INT_CTRL_3_REG, MMC_ST_ENM);
}

static uint32_t MMC5983MA_getMeasurementX()
{
    // Send command to device. TM_M self clears so we can access it directly.
    MMC5983MA_setRegisterBit(MMC_INT_CTRL_0_REG, MMC_TM_M);

    // Wait until measurement is completed
    do
    {
        // Wait a little so we won't flood MMC with requests
    	vTaskDelay(5 / portTICK_PERIOD_MS);
    } while (!MMC5983MA_isRegisterSet(MMC_STATUS_REG, MMC_MEAS_M_DONE));

    uint32_t temp = 0;
    uint32_t result = 0;
    uint8_t buffer[7] = {0};

    MMC5983MA_read(MMC_X_OUT_0_REG, buffer, 7);

    temp = (uint32_t) buffer[MMC_X_OUT_0_REG];
    temp = temp << MMC_XYZ_0_SHIFT;
    result |= temp;

    temp = (uint32_t) buffer[MMC_X_OUT_1_REG];
    temp = temp << MMC_XYZ_1_SHIFT;
    result |= temp;

    temp = (uint32_t) buffer[MMC_XYZ_OUT_2_REG];
    temp &= MMC_X2_MASK;
    temp = temp >> 6;
    result |= temp;
    return result;
}


static uint32_t MMC5983MA_getMeasurementY()
{
    // Send command to device. TM_M self clears so we can access it directly.
    MMC5983MA_setRegisterBit(MMC_INT_CTRL_0_REG, MMC_TM_M);

    // Wait until measurement is completed
    do
    {
        // Wait a little so we won't flood MMC with requests
    	vTaskDelay(5 / portTICK_PERIOD_MS);
    } while (!MMC5983MA_isRegisterSet(MMC_STATUS_REG, MMC_MEAS_M_DONE));

    uint32_t temp = 0;
    uint32_t result = 0;
    uint8_t registerValue = 0;

    registerValue = (MMC5983MA_readSingleByte(MMC_Y_OUT_0_REG));

    temp = (uint32_t) registerValue;
    temp = temp << MMC_XYZ_0_SHIFT;
    result |= temp;

    registerValue = (MMC5983MA_readSingleByte(MMC_Y_OUT_1_REG));

    temp = (uint32_t) registerValue;
    temp = temp << MMC_XYZ_1_SHIFT;
    result |= temp;

    registerValue = (MMC5983MA_readSingleByte(MMC_XYZ_OUT_2_REG));
    temp = (uint32_t) registerValue;
    temp &= MMC_Y2_MASK;
    temp = temp >> 4;
    result |= temp;
    return result;
}

static uint32_t MMC5983MA_getMeasurementZ()
{
    // Send command to device. TM_M self clears so we can access it directly.
    MMC5983MA_setRegisterBit(MMC_INT_CTRL_0_REG, MMC_TM_M);

    // Wait until measurement is completed
    do
    {
        // Wait a little so we won't flood MMC with requests
    	vTaskDelay(5 / portTICK_PERIOD_MS);
    } while (!MMC5983MA_isRegisterSet(MMC_STATUS_REG, MMC_MEAS_M_DONE));

    uint32_t temp = 0;
    uint32_t result = 0;
    uint8_t registerValue = 0;

    registerValue = (MMC5983MA_readSingleByte(MMC_Z_OUT_0_REG));

    temp = (uint32_t) registerValue;
    temp = temp << MMC_XYZ_0_SHIFT;
    result |= temp;

    registerValue = (MMC5983MA_readSingleByte(MMC_Z_OUT_1_REG));

    temp = (uint32_t) registerValue;
    temp = temp << MMC_XYZ_1_SHIFT;
    result |= temp;

    registerValue = (MMC5983MA_readSingleByte(MMC_XYZ_OUT_2_REG));

    temp = (uint32_t) registerValue;
    temp &= MMC_Z2_MASK;
    temp = temp >> 2;
    result |= temp;
    return result;
}
#endif
