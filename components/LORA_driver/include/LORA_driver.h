#pragma once

#define LORA_TX_NO_WAIT 0

typedef struct {
	int8_t tx_dbm;
	uint32_t freq_hz;
	uint8_t network_mode;
	uint64_t security_key;
} Lora_settings_t;


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

/**
* @brief Blocks until TX_DONE IRQ is set or timeout expires.
* @param[in] timeout_ms Maximum wait time in milliseconds.
* @return ESP_OK if TX completed, ESP_ERR_TIMEOUT otherwise.
*/
esp_err_t LORA_waitTXDone(uint32_t timeout_ms);

/**
* @brief Puts the radio in continuous RX mode (non-blocking).
* @return ESP_OK on success.
*/
esp_err_t LORA_startRX(void);

/**
* @brief Non-blocking check for a received packet.
* @param[out] rxbuffer Buffer to write received bytes into (must be at least 256 bytes).
* @param[out] size Number of bytes received.
* @return ESP_OK if a valid packet was received, ESP_ERR_NOT_FOUND if no packet yet,
*         ESP_FAIL if a packet arrived but failed CRC.
*/
esp_err_t LORA_receivePacketLoRa(uint8_t *rxbuffer, uint8_t *size);

/**
* @brief Non-blocking receive: polls for an incoming packet and, on success,
*        copies data to rxbuffer and re-enters RX mode automatically.
* @param[out] rxbuffer Buffer for received bytes (must be at least 256 bytes).
* @param[out] size Number of bytes received.
* @return ESP_OK if a valid packet was received, ESP_ERR_NOT_FOUND if nothing yet,
*         ESP_FAIL if a packet arrived but failed CRC.
*/
esp_err_t LORA_receive(uint8_t *rxbuffer, uint8_t *size);

/**
* @brief Performs Channel Activity Detection (CAD) for Listen Before Talk.
* @return ESP_OK if channel is clear (safe to transmit), ESP_FAIL if channel is busy.
*/
esp_err_t LORA_performCAD(void);

/**
* @brief Listen Before Talk transmit: performs up to 5 CAD checks, transmits if clear,
*        then re-enters RX mode. Drops the packet and logs a warning if channel stays busy.
* @param[in] txbuffer Data to transmit.
* @param[in] size Number of bytes to transmit.
* @return ESP_OK if transmitted successfully, ESP_FAIL if channel was busy.
*/
esp_err_t LORA_sendWithLBT(uint8_t *txbuffer, uint8_t size);
esp_err_t LORA_CW();
esp_err_t LORA_setRx();
int16_t   LORA_get_rssi();
