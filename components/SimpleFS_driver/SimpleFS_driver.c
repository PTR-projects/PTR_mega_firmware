#include <stdio.h>
#include "esp_err.h"
#include "esp_log.h"
#include "sfs_api.h"
#include <string.h>
#include "esp_crc.h"
#include "SimpleFS_driver.h"

static sfs_info_t partition_info;
static uint8_t curr_filename = 0;
static uint32_t read_ptr = 0;
static uint32_t write_ptr = 0;

static uint16_t crc16(uint8_t *buf, uint32_t len);
static bool component_init_done = false;

esp_err_t SimpleFS_init(const char * label){
	simplefs_api_init(&partition_info, label);

	component_init_done = true;
	return ESP_OK;
}

esp_err_t SimpleFS_formatMemory(uint32_t key){
	if(!component_init_done){
		return ESP_FAIL;
	}

	if(key != SFS_MAGIC_KEY){
		return ESP_FAIL;
	}

	return simplefs_api_erase();
}

esp_err_t SimpleFS_appendPacket(void * buffer, uint32_t size){
	if(!component_init_done){
		return ESP_FAIL;
	}

	if(size > sizeof((sfs_packet_t*)0)->payload){
		return ESP_FAIL;
	}

	sfs_packet_t new_packet;
	memset(&new_packet, 0, sizeof(sfs_packet_t));
	memcpy(&(new_packet.payload), buffer, size);

	new_packet.header.pre = SFS_HEADER_PRE;
	new_packet.header.filenum = curr_filename;
	new_packet.header.packet_len = (uint8_t)sizeof(sfs_packet_t);
	new_packet.CRC16 = crc16((void*)&new_packet, sizeof(sfs_packet_t) - sizeof((sfs_packet_t*)0)->CRC16);

	return ESP_OK;
}

esp_err_t SimpleFS_readMode(){

	return ESP_OK;
}

esp_err_t SimpleFS_writeMode(){

	return ESP_OK;
}

uint32_t SimpleFS_readMemory(uint32_t chunk_size, void * buffer){
	if((chunk_size == 0)
			|| ((chunk_size + read_ptr) > partition_info.partition_size_B)
			|| (chunk_size > SFS_MAX_CHUNK_SIZE_B)){
		return ESP_ERR_INVALID_SIZE;
	}

	// Check if read pointer is aligned to page
	if(read_ptr % sizeof(sfs_packet_t)){
		return ESP_ERR_INVALID_STATE;
	}

	// Align chunk size to SFS packet size
	chunk_size = chunk_size - chunk_size % sizeof(sfs_packet_t);

	// Create tmp buffer to store raw read
	uint8_t tmp_buffer[chunk_size];

	// Read raw data from memory
	simplefs_api_read(read_ptr, tmp_buffer, chunk_size);

	// Trim data
	// First check if last read Byte is empty (FF)
	if(tmp_buffer[sizeof(tmp_buffer)-1] == 0xFF) {
		for(uint8_t i=0; i<(chunk_size/sizeof(sfs_packet_t));i++){
			if(((sfs_packet_t*)(&tmp_buffer[i*sizeof(sfs_packet_t)]))->header.pre != SFS_HEADER_PRE){
				chunk_size = (i)*sizeof(sfs_packet_t);
				break;
			}
		}
	}

	read_ptr += chunk_size;

	return chunk_size;
}

static uint16_t crc16(uint8_t *buf, uint32_t len){
	return esp_crc16_le(UINT16_MAX, (uint8_t const *)buf, len);
}
