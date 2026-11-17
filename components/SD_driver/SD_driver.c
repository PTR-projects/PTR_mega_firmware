#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <sys/stat.h>
#include <unistd.h>

#include "esp_log.h"
#include "esp_err.h"
#include "BOARD_cfg.h"
#include "SD_driver.h"

#if !defined(SD_ENABLED)

esp_err_t SD_init(void){
	return ESP_OK;
}

esp_err_t SD_exportFlightLog(void){
	return ESP_ERR_NOT_SUPPORTED;
}

#else

#include "esp_crc.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "SimpleFS_driver.h"
#include "DataManager.h"
#include "LogSchema.h"

static const char *TAG = "SD_driver";

#define SD_MOUNT_POINT		"/sdcard"
#define SD_READ_CHUNK_B		4096

static sdmmc_card_t *s_card = NULL;
static bool s_mounted = false;

static const char *CSV_HEADER =
	"sys_time;"
	"accX;accY;accZ;"
	"gyroX;gyroY;gyroZ;"
	"magX;magY;magZ;"
	"accHX;accHY;accHZ;"
	"pressure;temp;"
	"latitude;longitude;altitude_gnss;gnss_sats;gnss_fix;"
	"altitude_press;altitude_kalman;ascent_rate_kalman;tilt;"
	"q0;q1;q2;q3;"
	"flightstate;"
	"ign1_cont;ign2_cont;ign3_cont;ign4_cont;"
	"ign1_state;ign2_state;ign3_state;ign4_state;"
	"vbat_mV;"
	"servo_1;servo_2;servo_3;servo_4;servo_en";

esp_err_t SD_init(void){
	if(s_mounted){
		return ESP_OK;
	}

	esp_vfs_fat_sdmmc_mount_config_t mount_config = {
		.format_if_mount_failed = false,
		.max_files = 5,
		.allocation_unit_size = 16 * 1024
	};

	sdmmc_host_t host = SDMMC_HOST_DEFAULT();
	sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
	slot_config.width = 4;
#if SOC_SDMMC_USE_GPIO_MATRIX
	slot_config.clk = SD_CLK_PIN;
	slot_config.cmd = SD_CMD_PIN;
	slot_config.d0  = SD_DAT0_PIN;
	slot_config.d1  = SD_DAT1_PIN;
	slot_config.d2  = SD_DAT2_PIN;
	slot_config.d3  = SD_DAT3_PIN;
#endif
	slot_config.cd = SD_DETECT_PIN;
	slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

	esp_err_t ret = esp_vfs_fat_sdmmc_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &s_card);
	if(ret != ESP_OK){
		if(ret == ESP_FAIL){
			ESP_LOGE(TAG, "Failed to mount filesystem");
		} else {
			ESP_LOGE(TAG, "Failed to initialize SD card: %s", esp_err_to_name(ret));
		}
		return ret;
	}

	s_mounted = true;
	ESP_LOGI(TAG, "SD card mounted at %s", SD_MOUNT_POINT);
	sdmmc_card_print_info(stdout, s_card);
	return ESP_OK;
}

static bool SD_pathExists(const char *path){
	struct stat st;
	return stat(path, &st) == 0;
}

/**
 * @brief Find next free NNN by probing %03u.bin / %03u.csv existence
 *        (same idea as FatFS FA_CREATE_NEW + FR_EXIST increment).
 */
static int SD_findNextIndex(void){
	for(int file_num = 0; file_num <= 999; file_num++){
		char bin_path[40];
		char csv_path[40];
		snprintf(bin_path, sizeof(bin_path), "%s/%03u.bin", SD_MOUNT_POINT, (unsigned)file_num);
		snprintf(csv_path, sizeof(csv_path), "%s/%03u.csv", SD_MOUNT_POINT, (unsigned)file_num);

		if(SD_pathExists(bin_path) || SD_pathExists(csv_path)){
			continue;
		}

		/* Confirm exclusivity the FatFS way: create-new must succeed */
		FILE *f = fopen(bin_path, "wbx");
		if(f == NULL){
			/* Race or unexpected; treat as taken and keep looking */
			continue;
		}
		fclose(f);
		unlink(bin_path);
		ESP_LOGI(TAG, "Next free file index: %03u", (unsigned)file_num);
		return file_num;
	}

	ESP_LOGE(TAG, "No free NNN index (000-999 full)");
	return -1;
}

static esp_err_t SD_writeBinFile(const char *path){
	FILE *f = fopen(path, "wb");
	if(f == NULL){
		ESP_LOGE(TAG, "fopen %s failed", path);
		return ESP_FAIL;
	}

	SimpleFS_readMode();
	SimpleFS_resetReadPointer();

	uint8_t *chunk = malloc(SD_READ_CHUNK_B);
	if(chunk == NULL){
		fclose(f);
		SimpleFS_writeMode();
		return ESP_ERR_NO_MEM;
	}

	esp_err_t err = ESP_OK;
	int32_t n;
	do {
		n = SimpleFS_readMemory(SD_READ_CHUNK_B, chunk);
		if(n < 0){
			ESP_LOGE(TAG, "SimpleFS_readMemory failed");
			err = ESP_FAIL;
			break;
		}
		if(n > 0){
			if(fwrite(chunk, 1, (size_t)n, f) != (size_t)n){
				ESP_LOGE(TAG, "fwrite bin failed");
				err = ESP_FAIL;
				break;
			}
		}
	} while(n > 0);

	free(chunk);
	fclose(f);
	SimpleFS_writeMode();
	return err;
}

_Static_assert(sizeof(((sfs_packet_t *)0)->payload) == LOG_SCHEMA_MAX_PAYLOAD, "schema payload size");

typedef struct {
	char name[LOG_SCHEMA_NAME_MAX + 1];
	uint8_t type;
	uint8_t unit;
	uint8_t bit_off;
	uint8_t bit_len;
	uint8_t advance;
	float scale;
	float offset;
} sd_schema_field_t;

static sd_schema_field_t sd_fields[LOG_SCHEMA_MAX_FIELDS];
static uint16_t sd_field_count;
static bool sd_schema_ready;

static uint8_t sd_asm_blob[LOG_SCHEMA_BLOB_MAX];
static uint16_t sd_asm_filled;
static uint16_t sd_asm_blob_bytes;
static uint16_t sd_asm_blob_crc;
static uint16_t sd_asm_field_count;
static uint16_t sd_asm_sample_bytes;
static uint8_t sd_asm_chunk_count;
static uint8_t sd_asm_next;
static bool sd_asm_active;

static bool SD_frameCrcOk(const sfs_packet_t *pkt){
	uint16_t crc = SimpleFS_crc16((uint8_t *)pkt, sizeof(sfs_packet_t) - sizeof(pkt->CRC16));
	return crc == pkt->CRC16;
}

static uint8_t SD_typeSize(uint8_t type){
	switch(type){
	case LOG_TYPE_I8:
	case LOG_TYPE_U8:
		return 1;
	case LOG_TYPE_I16:
	case LOG_TYPE_U16:
	case LOG_TYPE_F16:
		return 2;
	case LOG_TYPE_I32:
	case LOG_TYPE_U32:
	case LOG_TYPE_F32:
		return 4;
	case LOG_TYPE_F64:
		return 8;
	default:
		return 0;
	}
}

static float SD_float16(uint16_t bits){
	uint16_t exp = (bits >> 10) & 0x1Fu;
	uint16_t frac = bits & 0x03FFu;
	float magnitude;
	float sign = (bits & 0x8000u) ? -1.0f : 1.0f;

	if(exp == 0){
		magnitude = ldexpf((float)frac / 1024.0f, -14);
	} else if(exp == 31){
		magnitude = ldexpf(1.0f, 128);
	} else {
		magnitude = ldexpf(1.0f + (float)frac / 1024.0f, (int)exp - 15);
	}

	return sign * magnitude;
}

static void SD_resetAssembly(void){
	sd_asm_active = false;
	sd_asm_filled = 0;
	sd_asm_next = 0;
}

static esp_err_t SD_installSchema(void){
	uint16_t pos = 0;
	uint16_t cursor = 0;

	if(sd_asm_field_count == 0 || sd_asm_field_count > LOG_SCHEMA_MAX_FIELDS){
		return ESP_FAIL;
	}
	if(sd_asm_sample_bytes == 0 || sd_asm_sample_bytes > LOG_SCHEMA_MAX_PAYLOAD){
		return ESP_FAIL;
	}

	for(uint16_t i = 0; i < sd_asm_field_count; i++){
		uint8_t name_len;
		uint8_t size;

		if(pos >= sd_asm_blob_bytes){
			return ESP_FAIL;
		}
		name_len = sd_asm_blob[pos++];
		if(name_len < 1 || name_len > LOG_SCHEMA_NAME_MAX){
			return ESP_FAIL;
		}
		if((uint32_t)pos + name_len + 5u + 8u > sd_asm_blob_bytes){
			return ESP_FAIL;
		}

		memcpy(sd_fields[i].name, &sd_asm_blob[pos], name_len);
		sd_fields[i].name[name_len] = '\0';
		pos = (uint16_t)(pos + name_len);

		sd_fields[i].type = sd_asm_blob[pos++];
		sd_fields[i].unit = sd_asm_blob[pos++];
		sd_fields[i].bit_off = sd_asm_blob[pos++];
		sd_fields[i].bit_len = sd_asm_blob[pos++];
		sd_fields[i].advance = sd_asm_blob[pos++];
		memcpy(&sd_fields[i].scale, &sd_asm_blob[pos], sizeof(float));
		pos = (uint16_t)(pos + sizeof(float));
		memcpy(&sd_fields[i].offset, &sd_asm_blob[pos], sizeof(float));
		pos = (uint16_t)(pos + sizeof(float));

		size = SD_typeSize(sd_fields[i].type);
		if(size == 0){
			return ESP_FAIL;
		}
		if(sd_fields[i].bit_len != 0 &&
				((uint16_t)sd_fields[i].bit_off + sd_fields[i].bit_len) > ((uint16_t)size * 8u)){
			return ESP_FAIL;
		}
		if((uint32_t)cursor + size > sd_asm_sample_bytes){
			return ESP_FAIL;
		}
		cursor = (uint16_t)(cursor + sd_fields[i].advance);
		if(cursor > sd_asm_sample_bytes){
			return ESP_FAIL;
		}
	}

	if(pos != sd_asm_blob_bytes){
		return ESP_FAIL;
	}

	sd_field_count = sd_asm_field_count;
	return ESP_OK;
}

static void SD_acceptSchema(const uint8_t *payload, uint16_t payload_len){
	log_schema_chunk_hdr_t hdr;
	uint32_t blob_offset;
	uint16_t slice;

	if(payload_len < sizeof(hdr)){
		return;
	}

	memcpy(&hdr, payload, sizeof(hdr));
	if(hdr.magic != LOG_SCHEMA_MAGIC){
		return;
	}
	if(hdr.version != LOG_SCHEMA_VERSION || hdr.flags != LOG_SCHEMA_FLAG_LE){
		ESP_LOGW(TAG, "Ignoring schema version %u flags 0x%02X", hdr.version, hdr.flags);
		SD_resetAssembly();
		return;
	}
	if(hdr.chunk_count == 0 || hdr.chunk_index >= hdr.chunk_count){
		SD_resetAssembly();
		return;
	}

	if(hdr.chunk_index == 0){
		SD_resetAssembly();
		sd_asm_active = true;
		sd_asm_chunk_count = hdr.chunk_count;
		sd_asm_blob_bytes = hdr.blob_bytes;
		sd_asm_blob_crc = hdr.blob_crc;
		sd_asm_field_count = hdr.field_count;
		sd_asm_sample_bytes = hdr.sample_bytes;
		sd_asm_next = 0;
	}

	if(!sd_asm_active ||
			hdr.chunk_index != sd_asm_next ||
			hdr.chunk_count != sd_asm_chunk_count ||
			hdr.blob_bytes != sd_asm_blob_bytes ||
			hdr.blob_crc != sd_asm_blob_crc ||
			hdr.field_count != sd_asm_field_count ||
			hdr.sample_bytes != sd_asm_sample_bytes){
		SD_resetAssembly();
		return;
	}

	blob_offset = (uint32_t)hdr.chunk_index * LOG_SCHEMA_SLICE;
	if(blob_offset >= hdr.blob_bytes || hdr.blob_bytes > LOG_SCHEMA_BLOB_MAX){
		SD_resetAssembly();
		return;
	}

	slice = (uint16_t)(hdr.blob_bytes - blob_offset);
	if(slice > LOG_SCHEMA_SLICE){
		slice = LOG_SCHEMA_SLICE;
	}
	if((uint32_t)sizeof(hdr) + slice > payload_len){
		SD_resetAssembly();
		return;
	}
	if((uint32_t)sd_asm_filled + slice > hdr.blob_bytes){
		SD_resetAssembly();
		return;
	}

	memcpy(&sd_asm_blob[sd_asm_filled], payload + sizeof(hdr), slice);
	sd_asm_filled = (uint16_t)(sd_asm_filled + slice);
	sd_asm_next++;

	if(sd_asm_next != sd_asm_chunk_count){
		return;
	}
	if(sd_asm_filled != sd_asm_blob_bytes ||
			SimpleFS_crc16(sd_asm_blob, sd_asm_filled) != sd_asm_blob_crc ||
			SD_installSchema() != ESP_OK){
		ESP_LOGW(TAG, "Schema rejected");
		SD_resetAssembly();
		return;
	}

	sd_schema_ready = true;
	ESP_LOGI(TAG, "Schema installed: %u fields", sd_field_count);
	SD_resetAssembly();
}

static bool SD_readMeas(const uint8_t *payload, uint16_t payload_len, uint16_t cursor,
		const sd_schema_field_t *field, double *meas){
	uint8_t size = SD_typeSize(field->type);
	const uint8_t *p;

	if(size == 0 || (uint32_t)cursor + size > payload_len){
		return false;
	}

	p = payload + cursor;
	if(field->bit_len != 0){
		uint64_t word = 0;
		uint64_t mask;
		for(uint8_t i = 0; i < size; i++){
			word |= ((uint64_t)p[i]) << (8u * i);
		}
		mask = (field->bit_len >= 64) ? UINT64_MAX : ((1ULL << field->bit_len) - 1ULL);
		word = (word >> field->bit_off) & mask;
		*meas = (double)word;
		return true;
	}

	switch(field->type){
	case LOG_TYPE_I8: {
		int8_t v;
		memcpy(&v, p, sizeof(v));
		*meas = v;
		break;
	}
	case LOG_TYPE_U8: {
		uint8_t v;
		memcpy(&v, p, sizeof(v));
		*meas = v;
		break;
	}
	case LOG_TYPE_I16: {
		int16_t v;
		memcpy(&v, p, sizeof(v));
		*meas = v;
		break;
	}
	case LOG_TYPE_U16: {
		uint16_t v;
		memcpy(&v, p, sizeof(v));
		*meas = v;
		break;
	}
	case LOG_TYPE_I32: {
		int32_t v;
		memcpy(&v, p, sizeof(v));
		*meas = v;
		break;
	}
	case LOG_TYPE_U32: {
		uint32_t v;
		memcpy(&v, p, sizeof(v));
		*meas = v;
		break;
	}
	case LOG_TYPE_F16: {
		uint16_t v;
		memcpy(&v, p, sizeof(v));
		*meas = SD_float16(v);
		break;
	}
	case LOG_TYPE_F32: {
		float v;
		memcpy(&v, p, sizeof(v));
		*meas = v;
		break;
	}
	case LOG_TYPE_F64: {
		double v;
		memcpy(&v, p, sizeof(v));
		*meas = v;
		break;
	}
	default:
		return false;
	}
	return true;
}

static void SD_printMeas(FILE *f, const sd_schema_field_t *field, double meas){
	bool is_float = (field->type == LOG_TYPE_F16 || field->type == LOG_TYPE_F32 || field->type == LOG_TYPE_F64);
	double value = meas * (double)field->scale - (double)field->offset;

	if(!is_float && field->scale == 1.0f && field->offset == 0.0f){
		if(field->type == LOG_TYPE_I8 || field->type == LOG_TYPE_I16 || field->type == LOG_TYPE_I32){
			fprintf(f, "%ld", (long)meas);
		} else {
			fprintf(f, "%lu", (unsigned long)meas);
		}
		return;
	}

	fprintf(f, "%.6g", value);
}

static void SD_writeSchemaHeader(FILE *f){
	for(uint16_t i = 0; i < sd_field_count; i++){
		if(i > 0){
			fputc(';', f);
		}
		fputs(sd_fields[i].name, f);
	}
	fputc('\n', f);
}

static bool SD_writeSchemaLine(FILE *f, const uint8_t *payload, uint16_t payload_len){
	uint16_t cursor = 0;

	for(uint16_t i = 0; i < sd_field_count; i++){
		double meas = 0;
		if(!SD_readMeas(payload, payload_len, cursor, &sd_fields[i], &meas)){
			return false;
		}
		if(i > 0){
			fputc(';', f);
		}
		SD_printMeas(f, &sd_fields[i], meas);
		cursor = (uint16_t)(cursor + sd_fields[i].advance);
	}
	fputc('\n', f);
	return true;
}

static void SD_writeCsvLine(FILE *f, const DataPackage_t *p){
	uint8_t gnss_raw = (uint8_t)p->sensors.gnss_fix;
	uint8_t gnss_sats = gnss_raw & 0x3F;
	uint8_t gnss_fix = (gnss_raw >> 6) & 0x03;

	fprintf(f,
		"%lu;"
		"%.6g;%.6g;%.6g;"
		"%.6g;%.6g;%.6g;"
		"%.6g;%.6g;%.6g;"
		"%.6g;%.6g;%.6g;"
		"%.6g;%d;"
		"%.6g;%.6g;%.6g;%u;%u;"
		"%.6g;%.6g;%.6g;%u;"
		"%.6g;%.6g;%.6g;%.6g;"
		"%u;"
		"%u;%u;%u;%u;"
		"%u;%u;%u;%u;"
		"%u;"
		"%d;%d;%d;%d;%u\n",
		(unsigned long)p->sys_time,
		p->sensors.accX, p->sensors.accY, p->sensors.accZ,
		p->sensors.gyroX, p->sensors.gyroY, p->sensors.gyroZ,
		p->sensors.magX, p->sensors.magY, p->sensors.magZ,
		p->sensors.accHX, p->sensors.accHY, p->sensors.accHZ,
		p->sensors.pressure, (int)p->sensors.temp,
		p->sensors.latitude, p->sensors.longitude, p->sensors.altitude_gnss, gnss_sats, gnss_fix,
		p->ahrs.altitude_press, p->ahrs.altitude_kalman, p->ahrs.ascent_rate_kalman, p->ahrs.tilt,
		p->ahrs.q0, p->ahrs.q1, p->ahrs.q2, p->ahrs.q3,
		p->flightstate,
		p->ign.ign1_cont, p->ign.ign2_cont, p->ign.ign3_cont, p->ign.ign4_cont,
		p->ign.ign1_state, p->ign.ign2_state, p->ign.ign3_state, p->ign.ign4_state,
		p->vbat_mV,
		p->servo.servo_1, p->servo.servo_2, p->servo.servo_3, p->servo.servo_4, p->servo.servo_en
	);
}

static esp_err_t SD_writeCsvFile(const char *path){
	FILE *f = fopen(path, "w");
	if(f == NULL){
		ESP_LOGE(TAG, "fopen %s failed", path);
		return ESP_FAIL;
	}

	SimpleFS_readMode();
	SimpleFS_resetReadPointer();

	sd_schema_ready = false;
	sd_field_count = 0;
	SD_resetAssembly();

	sfs_packet_t pkt;
	esp_err_t err = ESP_OK;
	bool header_written = false;
	int32_t n;
	do {
		n = SimpleFS_readMemory(sizeof(sfs_packet_t), &pkt);
		if(n < 0){
			ESP_LOGE(TAG, "SimpleFS_readMemory failed (csv)");
			err = ESP_FAIL;
			break;
		}
		if(n == 0){
			break;
		}
		if((size_t)n < sizeof(sfs_packet_t)){
			break;
		}
		if(!SD_frameCrcOk(&pkt)){
			continue;
		}

		if(pkt.header.pre == SFS_HEADER_SCHEMA){
			SD_acceptSchema(pkt.payload, sizeof(pkt.payload));
			continue;
		}
		if(pkt.header.pre != SFS_HEADER_PRE){
			continue;
		}

		if(!header_written){
			if(sd_schema_ready){
				SD_writeSchemaHeader(f);
			} else {
				fprintf(f, "%s\n", CSV_HEADER);
			}
			header_written = true;
		}

		if(sd_schema_ready){
			if(!SD_writeSchemaLine(f, pkt.payload, sizeof(pkt.payload))){
				ESP_LOGE(TAG, "CSV schema decode failed");
				err = ESP_FAIL;
				break;
			}
		} else {
			DataPackage_t pkg;
			memset(&pkg, 0, sizeof(pkg));
			size_t copy_len = sizeof(DataPackage_t);
			if(copy_len > sizeof(pkt.payload)){
				copy_len = sizeof(pkt.payload);
			}
			memcpy(&pkg, pkt.payload, copy_len);
			SD_writeCsvLine(f, &pkg);
		}
	} while(n > 0);

	fclose(f);
	SimpleFS_writeMode();
	return err;
}

esp_err_t SD_exportFlightLog(void){
	if(!s_mounted){
		esp_err_t init_err = SD_init();
		if(init_err != ESP_OK){
			return init_err;
		}
	}

	int idx = SD_findNextIndex();
	if(idx < 0){
		return ESP_FAIL;
	}

	char bin_path[32];
	char csv_path[32];
	snprintf(bin_path, sizeof(bin_path), "%s/%03d.bin", SD_MOUNT_POINT, idx);
	snprintf(csv_path, sizeof(csv_path), "%s/%03d.csv", SD_MOUNT_POINT, idx);

	ESP_LOGI(TAG, "Exporting flight log to %s and %s (%lu bytes)",
		bin_path, csv_path, (unsigned long)SimpleFS_getFileSize());

	esp_err_t err = SD_writeBinFile(bin_path);
	if(err != ESP_OK){
		unlink(bin_path);
		return err;
	}

	err = SD_writeCsvFile(csv_path);
	if(err != ESP_OK){
		unlink(csv_path);
		return err;
	}

	ESP_LOGI(TAG, "Flight log export complete");
	return ESP_OK;
}

#endif /* SD_ENABLED */
