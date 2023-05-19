#pragma once

#define LORA_TX_NO_WAIT 0

esp_err_t LORA_init();
esp_err_t LORA_setupLoRaTX(uint32_t frequency, int32_t offset, uint8_t modParam1,
				uint8_t modParam2, uint8_t modParam3, uint8_t modParam4, uint8_t device);
esp_err_t LORA_sendPacketLoRa(uint8_t *txbuffer, uint16_t size, uint32_t txtimeout);
esp_err_t LORA_CW();
esp_err_t LORA_setRx();
esp_err_t LORA_performCAD();
int16_t   LORA_get_rssi();
