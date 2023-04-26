#pragma once

#define LORA_TX_NO_WAIT 0

/**
* @brief Initializes the LORA module to default settings.
* @details This function initializes the SX126X, sets the LORA mode to 433MHz and 0dBm, and waits for 20ms.
*/
esp_err_t LORA_init();

/**
* @brief Configures the LORA module for TX mode and sets the specified parameters.
* @param[in] frequency The frequency to transmit on, in Hz.
* @param[in] offset The frequency offset, in Hz.
* @param[in] modParam1 The first modulation parameter. LoRa Spread Factor.
* @param[in] modParam2 The second modulation parameter. LoRa Bandwidth.
* @param[in] modParam3 The third modulation parameter. LoRa Coding Rate.
* @param[in] modParam4 The fourth modulation parameter. Low DataRate Optimization configuration.
* @param[in] device The device to transmit with. Default 0.
*/
esp_err_t LORA_setupLoRaTX(uint32_t frequency, int32_t offset, uint8_t modParam1,
				uint8_t modParam2, uint8_t modParam3, uint8_t modParam4, uint8_t device);

/**
* @brief Sends a data packet over the LORA module in TX mode.
* @param[in] txbuffer The buffer containing the data to be transmitted.
* @param[in] size The size of the data to be transmitted, in bytes.
* @param[in] txtimeout The timeout for the transmission, in milliseconds. Zero means no wait.
* @return True if the transmission was successful, false otherwise.
* This function configures the LORA module for TX mode, sets the specified parameters,
* sends the data package to the module, and starts the transmission. If the transmission is successful,
* the function returns true. If the transmission fails or times out, the function returns false.
*/
esp_err_t LORA_sendPacketLoRa(uint8_t *txbuffer, uint16_t size, uint32_t txtimeout);
