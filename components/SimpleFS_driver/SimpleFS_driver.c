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

static uint16_t crc16(uint8_t *buf, uint32_t len);
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
		return ESP_FAIL;
	}

	if(key != SFS_MAGIC_KEY){
		return ESP_FAIL;
	}

	if((access_locked_r == true) || (access_locked_w == true)){
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
	new_packet.CRC16 = crc16((void*)&new_packet, sizeof(sfs_packet_t) - sizeof((sfs_packet_t*)0)->CRC16);

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

	memcpy(buffer, tmp_buffer, chunk_size);

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
		return err;
	}

	if(tmp == 0xFF){
		write_ptr = curr_packet * packet_size;
		ESP_LOGV(ESP_SIMPLEFS_TAG, "Flash empty");
		return ESP_OK;
	}

	// Check packet at 50% of memory
	curr_packet = packet_max >> 1;
	err = SimpleFS_readMemoryLL(curr_packet * packet_size, 1, (void *)(&tmp));
	if(err == ESP_FAIL){
		ESP_LOGE(ESP_SIMPLEFS_TAG, "Read failed");
		return err;
	}

	if(tmp != 0xFF){
		write_ptr = curr_packet * packet_size;
		ESP_LOGI(ESP_SIMPLEFS_TAG, "Flash too full >50%%");
		ESP_LOGI(ESP_SIMPLEFS_TAG, "Data end: %iB", write_ptr);
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

static uint16_t IRAM_ATTR crc16(uint8_t *buf, uint32_t len){
	return esp_crc16_le(UINT16_MAX, (uint8_t const *)buf, len);
}
