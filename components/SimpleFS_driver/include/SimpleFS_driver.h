#pragma once

#include "esp_err.h"
#include "esp_log.h"

#define SFS_HEADER_PRE 0xAA55
#define SFS_MAGIC_KEY 0x08102023
#define SFS_MAX_CHUNK_SIZE_B 16384UL

typedef struct __attribute__((__packed__)){
	struct __attribute__((__packed__)){
		uint16_t pre;
		uint8_t filenum;
		uint8_t packet_len;
	} header;
	uint8_t payload[128-6];
	uint16_t CRC16;
} sfs_packet_t;

typedef struct{
	uint8_t filename;
	uint32_t start_pos;
	uint32_t size;
} sfs_file_stat_t;


esp_err_t SimpleFS_init(const char * label);
esp_err_t SimpleFS_formatMemory(uint32_t key);
esp_err_t SimpleFS_appendPacket(void * buffer, uint32_t size);
uint8_t SimpleFS_memoryUsedPercentage();
esp_err_t SimpleFS_readMode();
esp_err_t SimpleFS_writeMode();
int32_t SimpleFS_readMemory(uint32_t chunk_size, void * buffer);
int32_t SimpleFS_readMemoryLL(uint32_t position, uint32_t chunk_size, void * buffer);
void SimpleFS_resetReadPointer();
uint32_t SimpleFS_getFileSize();
