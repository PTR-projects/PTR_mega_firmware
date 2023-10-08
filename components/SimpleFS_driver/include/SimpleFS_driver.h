#pragma once

#define SFS_HEADER_PRE 0xAA55
#define SFS_MAGIC_KEY 0x08102023
#define SFS_MAX_CHUNK_SIZE_B 16384UL

typedef struct __attribute__((__packed__)){
	struct __attribute__((__packed__)){
		uint16_t pre;
		uint8_t filenum;
		uint8_t packet_len;
	} header;
	uint8_t payload[250];
	uint16_t CRC16;
} sfs_packet_t;

typedef struct{
	uint8_t filename;
	uint32_t start_pos;
	uint32_t size;
} sfs_file_stat_t;


esp_err_t SimpleFS_init(const char * label);
