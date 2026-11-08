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
static bool access_locked_r = false;
static bool access_locked_w = false;
static bool component_init_done = false;

const char ESP_SIMPLEFS_TAG[] = "SimpleFS";

static esp_err_t SimpleFS_findDataEnd();

esp_err_t SimpleFS_init(const char * label){
	esp_err_t err = ESP_OK;

	if((access_locked_r == true) || (access_locked_w == true)){
		return ESP_FAIL;
	}

	if(component_init_done == false){
		err = simplefs_api_init(&partition_info, label);
		component_init_done = true;

		// Reset Read and Write Pointers
		read_ptr  = 0;
		write_ptr = 0;

	} else {
		ESP_LOGI(ESP_SIMPLEFS_TAG, "SimpleFS already mounted. Skip API init.");
	}

	// Check if any data present in memory
	uint8_t tmp_buff = 0x00;

	if(SimpleFS_readMemoryLL(0, sizeof(tmp_buff), &tmp_buff) > 0){
		if(tmp_buff != 0xFF){
			ESP_LOGE(ESP_SIMPLEFS_TAG, "File present and not empty!");
			SimpleFS_findDataEnd();
			err = ESP_FAIL;
		}
	}

	return err;
}

esp_err_t IRAM_ATTR SimpleFS_formatMemory(uint32_t key, sfs_format_type_e type){
	if(!component_init_done){
		ESP_LOGE(ESP_SIMPLEFS_TAG, "Format - component not initialized");
		return ESP_FAIL;
	}

	if(key != SFS_MAGIC_KEY){
		ESP_LOGE(ESP_SIMPLEFS_TAG, "Format - magic key error");
		return ESP_FAIL;
	}

	if((access_locked_r == true) || (access_locked_w == true)){
		ESP_LOGE(ESP_SIMPLEFS_TAG, "Format - access locked W:%i - R:%i", (int)access_locked_w, (int)access_locked_r);
		return ESP_FAIL;
	}

	access_locked_r = true;
	access_locked_w = true;

	esp_err_t err = ESP_OK;

	if(type == SFS_FORMAT_ALL){
		err = simplefs_api_erase(0);
	}
	else if(type == SFS_FORMAT_RANGE) {
		err = simplefs_api_erase(write_ptr);
	}
	else {
		err = ESP_FAIL;
	}

	access_locked_r = false;
	access_locked_w = false;

	if(err == ESP_OK){
		write_ptr = 0;
	}
	return err;
}

esp_err_t IRAM_ATTR SimpleFS_appendPacket(void * buffer, uint32_t size){
	if(!component_init_done){
		ESP_LOGE(ESP_SIMPLEFS_TAG, "SimpleFS not initialized");
		return ESP_FAIL;
	}

	if(access_locked_w == true){
		return ESP_FAIL;
	}

	if(size > sizeof(((sfs_packet_t*)0)->payload)){
		ESP_LOGE(ESP_SIMPLEFS_TAG, "Write size bigger than packet payload");
		return ESP_FAIL;
	}

	ESP_LOGV(ESP_SIMPLEFS_TAG, "Write size (payload): %i", size);

	sfs_packet_t new_packet  __attribute__((aligned(4)));
	memset(&new_packet, 0, sizeof(sfs_packet_t));
	memcpy(&(new_packet.payload), buffer, size);

	new_packet.header.pre = SFS_HEADER_PRE;
	new_packet.header.filenum = curr_filename;
	new_packet.header.packet_len = sizeof(sfs_packet_t)/sizeof(uint32_t);
	new_packet.CRC16 = SimpleFS_crc16((void*)&new_packet, sizeof(sfs_packet_t) - sizeof((sfs_packet_t*)0)->CRC16);

	esp_err_t err = simplefs_api_prog(write_ptr, &new_packet, sizeof(sfs_packet_t));

	if(err == ESP_OK){
		write_ptr += sizeof(sfs_packet_t);
	}

	ESP_LOGV(ESP_SIMPLEFS_TAG, "Write pointer: %i", write_ptr);

	return err;
}

uint8_t SimpleFS_memoryUsedPercentage(){
	return (100*write_ptr) / partition_info.partition_size_B;
}

esp_err_t SimpleFS_readMode(){

	return ESP_OK;
}

esp_err_t SimpleFS_writeMode(){

	return ESP_OK;
}

int32_t IRAM_ATTR SimpleFS_readMemory(uint32_t chunk_size, void * buffer){
	if((chunk_size == 0)
			|| ((chunk_size + read_ptr) > partition_info.partition_size_B)
			|| (chunk_size > SFS_MAX_CHUNK_SIZE_B)
			|| (chunk_size < sizeof(sfs_packet_t))){
		return -1;
	}

	if(access_locked_r == true){
		return ESP_FAIL;
	}

	// Align chunk size to SFS packet size
	if(chunk_size > sizeof(sfs_packet_t)){
		chunk_size = chunk_size - chunk_size % sizeof(sfs_packet_t);
	}

	// Create tmp buffer to store raw read
	uint8_t tmp_buffer[chunk_size];

	// Read raw data from memory
	if(simplefs_api_read(read_ptr, tmp_buffer, chunk_size) != ESP_OK){
		return -1;
	}

	// Trim data
	// First check if last read Byte is empty (FF)
	if(tmp_buffer[sizeof(tmp_buffer)-1] == 0xFF) {
		ESP_LOGV(ESP_SIMPLEFS_TAG, "Trimm 0xFF");
		for(uint8_t i=0; i<(chunk_size/sizeof(sfs_packet_t));i++){
			if(((sfs_packet_t*)(&tmp_buffer[i*sizeof(sfs_packet_t)]))->header.pre != SFS_HEADER_PRE){
				chunk_size = (i)*sizeof(sfs_packet_t);
				break;
			}
		}
	}

	// Copy trimmed data to output buffer
	memcpy(buffer, tmp_buffer, chunk_size);

	// Move read pointer to new position
	read_ptr += chunk_size;

	return chunk_size;
}

int32_t IRAM_ATTR SimpleFS_dumpMemory(uint32_t chunk_size, void * buffer){
	if((chunk_size == 0)
			|| (chunk_size > SFS_MAX_CHUNK_SIZE_B)
			|| (chunk_size < sizeof(sfs_packet_t))){
		return -1;
	}

	if(access_locked_r == true){
		return ESP_FAIL;
	}

	// Trim length for last chunk
	if((chunk_size + read_ptr) > partition_info.partition_size_B){
		chunk_size = partition_info.partition_size_B - read_ptr;
	}

	// Read raw data from memory
	if(simplefs_api_read(read_ptr, buffer, chunk_size) != ESP_OK){
		return -1;
	}

	// Move read pointer to new position
	read_ptr += chunk_size;

	return chunk_size;
}

int32_t IRAM_ATTR SimpleFS_readMemoryLL(uint32_t position, uint32_t chunk_size, void * buffer){
	if((chunk_size == 0)
			|| ((chunk_size + read_ptr) > partition_info.partition_size_B)
			|| (chunk_size > SFS_MAX_CHUNK_SIZE_B)){
		return ESP_FAIL;
	}

	if(access_locked_r == true){
		ESP_LOGE(ESP_SIMPLEFS_TAG, "Access locked - readLL");
		return ESP_FAIL;
	}

	// Create tmp buffer to store raw read
	uint8_t tmp_buffer[chunk_size];

	// Read raw data from memory
	if(simplefs_api_read(position, tmp_buffer, chunk_size) != ESP_OK){
		return -1;
	}

	memcpy(buffer, tmp_buffer, chunk_size);

	return chunk_size;
}

void SimpleFS_resetReadPointer(){
	read_ptr = 0;
}

uint32_t SimpleFS_getFileSize(){
	return write_ptr;
}

static esp_err_t SimpleFS_findDataEnd(){
	if(access_locked_w == true){
		ESP_LOGE(ESP_SIMPLEFS_TAG, "Find Data End - access locked!");
		return ESP_FAIL;
	}

	access_locked_w = true;

	esp_err_t err 		  = ESP_OK;
	uint8_t   tmp         = 0x00;
	uint32_t  packet_size = sizeof(sfs_packet_t);
	uint32_t  packet_max  = partition_info.partition_size_B / packet_size;
	uint32_t  curr_packet = 0;

	ESP_LOGI(ESP_SIMPLEFS_TAG, "Max packet count: %i, packet size: %i, Flash size: %i", packet_max, packet_size, partition_info.partition_size_B);

	// Check first page
	curr_packet = 0;
	err = SimpleFS_readMemoryLL(curr_packet * packet_size, 1, (void *)(&tmp));
	if(err == ESP_FAIL){
		ESP_LOGE(ESP_SIMPLEFS_TAG, "Read failed");
		access_locked_w = false;
		return err;
	}

	if(tmp == 0xFF){
		write_ptr = curr_packet * packet_size;
		ESP_LOGV(ESP_SIMPLEFS_TAG, "Flash empty");
		access_locked_w = false;
		return ESP_OK;
	}

	// Check packet at 50% of memory
	curr_packet = packet_max >> 1;
	err = SimpleFS_readMemoryLL(curr_packet * packet_size, 1, (void *)(&tmp));
	if(err == ESP_FAIL){
		ESP_LOGE(ESP_SIMPLEFS_TAG, "Read failed");
		access_locked_w = false;
		return err;
	}

	if(tmp != 0xFF){
		write_ptr = curr_packet * packet_size;
		ESP_LOGI(ESP_SIMPLEFS_TAG, "Flash too full >50%%");
		ESP_LOGI(ESP_SIMPLEFS_TAG, "Data end: %iB", write_ptr);
		access_locked_w = false;
		return ESP_OK;
	}

	// Init search algorithm - look for data end between second page and 50% of the memory
	uint32_t curr_packet_min = 1;
	uint32_t curr_packet_max = packet_max >> 1;

	while(1){
		// Check in the middle of search range
		curr_packet = (curr_packet_max + curr_packet_min)>>1;

		// Check if search range has more than 2 packets
		if((curr_packet_max-curr_packet_min) <= 1 ){
			ESP_LOGV(ESP_SIMPLEFS_TAG, "End found");
			break;
		}

		// Read selected packet (first byte only) and check if it is empty (0xFF)
		SimpleFS_readMemoryLL(curr_packet*packet_size, 1, (void *)(&tmp));
		if(err == ESP_FAIL){
			ESP_LOGE(ESP_SIMPLEFS_TAG, "Read failed");
			access_locked_w = false;
			return err;
		}
		ESP_LOGV(ESP_SIMPLEFS_TAG, "Ptr: %i, val: 0x%x", curr_packet*packet_size, tmp);

		if(tmp == 0xFF)
			curr_packet_max = curr_packet;	// Byte cleared - move upper search boundry to this position
		else
			curr_packet_min = curr_packet;	// Byte written - move lower search boundry to this position
	}

	SimpleFS_readMemoryLL(curr_packet*packet_size, 1, (void *)(&tmp));
	if(tmp != 0xFF)
		write_ptr = (curr_packet + 1) * packet_size;
	else
		write_ptr = curr_packet * packet_size;

	access_locked_w = false;

	ESP_LOGI(ESP_SIMPLEFS_TAG, "Data end: %iB", write_ptr);

	return ESP_OK;
}

const uint16_t crc_tab16[256] =
{        
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040,
};

uint16_t IRAM_ATTR SimpleFS_crc16(uint8_t *buf, uint32_t len){
	uint16_t crc = 0xFFFF;

	while(len--){
		crc = (crc >> 8) ^ crc_tab16[(crc ^ *(buf++)) & 0xff];
	}

	return crc;
}
