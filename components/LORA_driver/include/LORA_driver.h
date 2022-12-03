#pragma once

#define LORA_TX_NO_WAIT 0

void LORA_init();
void LORA_setupLoRaTX(uint32_t frequency, int32_t offset, uint8_t modParam1,
	uint8_t modParam2, uint8_t modParam3, uint8_t modParam4, uint8_t device);
bool LORA_sendPacketLoRa(uint8_t *txbuffer, uint16_t size, uint32_t txtimeout);
