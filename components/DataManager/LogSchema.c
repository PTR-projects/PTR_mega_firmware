#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"

#include "CONFIG.h"
#include "DataManager.h"
#include "LogSchema.h"

static const char *TAG = "LogSchema";

/**
 * @brief One column of the measurement log, in wire order.
 * advance is the number of bytes the cursor moves after this column.
 */
typedef struct {
	const char *name;
	uint8_t type;
	uint8_t unit;
	uint8_t bit_off;
	uint8_t bit_len;
	uint8_t advance;
	float scale;
	float offset;
} log_field_def_t;

/* Same 43 CSV columns as the legacy parser. Scale 1 and offset 0 keep the numbers unchanged. */
static const log_field_def_t log_fields[] = {
	{ .name = "sys_time",            .type = LOG_TYPE_U32, .unit = LOG_UNIT_TICK_0_1MS, .bit_off = 0, .bit_len = 0, .advance = 4, .scale = 1.0f, .offset = 0.0f },
	{ .name = "accX",                .type = LOG_TYPE_F32, .unit = LOG_UNIT_G,          .bit_off = 0, .bit_len = 0, .advance = 4, .scale = 1.0f, .offset = 0.0f },
	{ .name = "accY",                .type = LOG_TYPE_F32, .unit = LOG_UNIT_G,          .bit_off = 0, .bit_len = 0, .advance = 4, .scale = 1.0f, .offset = 0.0f },
	{ .name = "accZ",                .type = LOG_TYPE_F32, .unit = LOG_UNIT_G,          .bit_off = 0, .bit_len = 0, .advance = 4, .scale = 1.0f, .offset = 0.0f },
	{ .name = "gyroX",               .type = LOG_TYPE_F32, .unit = LOG_UNIT_DPS,        .bit_off = 0, .bit_len = 0, .advance = 4, .scale = 1.0f, .offset = 0.0f },
	{ .name = "gyroY",               .type = LOG_TYPE_F32, .unit = LOG_UNIT_DPS,        .bit_off = 0, .bit_len = 0, .advance = 4, .scale = 1.0f, .offset = 0.0f },
	{ .name = "gyroZ",               .type = LOG_TYPE_F32, .unit = LOG_UNIT_DPS,        .bit_off = 0, .bit_len = 0, .advance = 4, .scale = 1.0f, .offset = 0.0f },
	{ .name = "magX",                .type = LOG_TYPE_F32, .unit = LOG_UNIT_RAW,        .bit_off = 0, .bit_len = 0, .advance = 4, .scale = 1.0f, .offset = 0.0f },
	{ .name = "magY",                .type = LOG_TYPE_F32, .unit = LOG_UNIT_RAW,        .bit_off = 0, .bit_len = 0, .advance = 4, .scale = 1.0f, .offset = 0.0f },
	{ .name = "magZ",                .type = LOG_TYPE_F32, .unit = LOG_UNIT_RAW,        .bit_off = 0, .bit_len = 0, .advance = 4, .scale = 1.0f, .offset = 0.0f },
	{ .name = "accHX",               .type = LOG_TYPE_F32, .unit = LOG_UNIT_G,          .bit_off = 0, .bit_len = 0, .advance = 4, .scale = 1.0f, .offset = 0.0f },
	{ .name = "accHY",               .type = LOG_TYPE_F32, .unit = LOG_UNIT_G,          .bit_off = 0, .bit_len = 0, .advance = 4, .scale = 1.0f, .offset = 0.0f },
	{ .name = "accHZ",               .type = LOG_TYPE_F32, .unit = LOG_UNIT_G,          .bit_off = 0, .bit_len = 0, .advance = 4, .scale = 1.0f, .offset = 0.0f },
	{ .name = "pressure",            .type = LOG_TYPE_F32, .unit = LOG_UNIT_PA,         .bit_off = 0, .bit_len = 0, .advance = 4, .scale = 1.0f, .offset = 0.0f },
	{ .name = "temp",                .type = LOG_TYPE_I8,  .unit = LOG_UNIT_DEGC,       .bit_off = 0, .bit_len = 0, .advance = 1, .scale = 1.0f, .offset = 0.0f },
	{ .name = "latitude",            .type = LOG_TYPE_F32, .unit = LOG_UNIT_DEG,        .bit_off = 0, .bit_len = 0, .advance = 4, .scale = 1.0f, .offset = 0.0f },
	{ .name = "longitude",           .type = LOG_TYPE_F32, .unit = LOG_UNIT_DEG,        .bit_off = 0, .bit_len = 0, .advance = 4, .scale = 1.0f, .offset = 0.0f },
	{ .name = "altitude_gnss",       .type = LOG_TYPE_F32, .unit = LOG_UNIT_M,          .bit_off = 0, .bit_len = 0, .advance = 4, .scale = 1.0f, .offset = 0.0f },
	{ .name = "gnss_sats",           .type = LOG_TYPE_U8,  .unit = LOG_UNIT_COUNT,      .bit_off = 0, .bit_len = 6, .advance = 0, .scale = 1.0f, .offset = 0.0f },
	{ .name = "gnss_fix",            .type = LOG_TYPE_U8,  .unit = LOG_UNIT_RAW,        .bit_off = 6, .bit_len = 2, .advance = 1, .scale = 1.0f, .offset = 0.0f },
	{ .name = "altitude_press",      .type = LOG_TYPE_F32, .unit = LOG_UNIT_M,          .bit_off = 0, .bit_len = 0, .advance = 4, .scale = 1.0f, .offset = 0.0f },
	{ .name = "altitude_kalman",     .type = LOG_TYPE_F32, .unit = LOG_UNIT_M,          .bit_off = 0, .bit_len = 0, .advance = 4, .scale = 1.0f, .offset = 0.0f },
	{ .name = "ascent_rate_kalman",  .type = LOG_TYPE_F32, .unit = LOG_UNIT_MPS,        .bit_off = 0, .bit_len = 0, .advance = 4, .scale = 1.0f, .offset = 0.0f },
	{ .name = "tilt",                .type = LOG_TYPE_U8,  .unit = LOG_UNIT_DEG,        .bit_off = 0, .bit_len = 0, .advance = 1, .scale = 1.0f, .offset = 0.0f },
	{ .name = "q0",                  .type = LOG_TYPE_F32, .unit = LOG_UNIT_NONE,       .bit_off = 0, .bit_len = 0, .advance = 4, .scale = 1.0f, .offset = 0.0f },
	{ .name = "q1",                  .type = LOG_TYPE_F32, .unit = LOG_UNIT_NONE,       .bit_off = 0, .bit_len = 0, .advance = 4, .scale = 1.0f, .offset = 0.0f },
	{ .name = "q2",                  .type = LOG_TYPE_F32, .unit = LOG_UNIT_NONE,       .bit_off = 0, .bit_len = 0, .advance = 4, .scale = 1.0f, .offset = 0.0f },
	{ .name = "q3",                  .type = LOG_TYPE_F32, .unit = LOG_UNIT_NONE,       .bit_off = 0, .bit_len = 0, .advance = 4, .scale = 1.0f, .offset = 0.0f },
	{ .name = "flightstate",         .type = LOG_TYPE_U8,  .unit = LOG_UNIT_RAW,        .bit_off = 0, .bit_len = 0, .advance = 1, .scale = 1.0f, .offset = 0.0f },
	{ .name = "ign1_cont",           .type = LOG_TYPE_U8,  .unit = LOG_UNIT_BOOL,       .bit_off = 0, .bit_len = 1, .advance = 0, .scale = 1.0f, .offset = 0.0f },
	{ .name = "ign2_cont",           .type = LOG_TYPE_U8,  .unit = LOG_UNIT_BOOL,       .bit_off = 1, .bit_len = 1, .advance = 0, .scale = 1.0f, .offset = 0.0f },
	{ .name = "ign3_cont",           .type = LOG_TYPE_U8,  .unit = LOG_UNIT_BOOL,       .bit_off = 2, .bit_len = 1, .advance = 0, .scale = 1.0f, .offset = 0.0f },
	{ .name = "ign4_cont",           .type = LOG_TYPE_U8,  .unit = LOG_UNIT_BOOL,       .bit_off = 3, .bit_len = 1, .advance = 0, .scale = 1.0f, .offset = 0.0f },
	{ .name = "ign1_state",          .type = LOG_TYPE_U8,  .unit = LOG_UNIT_BOOL,       .bit_off = 4, .bit_len = 1, .advance = 0, .scale = 1.0f, .offset = 0.0f },
	{ .name = "ign2_state",          .type = LOG_TYPE_U8,  .unit = LOG_UNIT_BOOL,       .bit_off = 5, .bit_len = 1, .advance = 0, .scale = 1.0f, .offset = 0.0f },
	{ .name = "ign3_state",          .type = LOG_TYPE_U8,  .unit = LOG_UNIT_BOOL,       .bit_off = 6, .bit_len = 1, .advance = 0, .scale = 1.0f, .offset = 0.0f },
	{ .name = "ign4_state",          .type = LOG_TYPE_U8,  .unit = LOG_UNIT_BOOL,       .bit_off = 7, .bit_len = 1, .advance = 1, .scale = 1.0f, .offset = 0.0f },
	{ .name = "vbat_mV",             .type = LOG_TYPE_U16, .unit = LOG_UNIT_MV,         .bit_off = 0, .bit_len = 0, .advance = 2, .scale = 1.0f, .offset = 0.0f },
	{ .name = "servo_1",             .type = LOG_TYPE_I8,  .unit = LOG_UNIT_PERCENT,    .bit_off = 0, .bit_len = 0, .advance = 1, .scale = 1.0f, .offset = 0.0f },
	{ .name = "servo_2",             .type = LOG_TYPE_I8,  .unit = LOG_UNIT_PERCENT,    .bit_off = 0, .bit_len = 0, .advance = 1, .scale = 1.0f, .offset = 0.0f },
	{ .name = "servo_3",             .type = LOG_TYPE_I8,  .unit = LOG_UNIT_PERCENT,    .bit_off = 0, .bit_len = 0, .advance = 1, .scale = 1.0f, .offset = 0.0f },
	{ .name = "servo_4",             .type = LOG_TYPE_I8,  .unit = LOG_UNIT_PERCENT,    .bit_off = 0, .bit_len = 0, .advance = 1, .scale = 1.0f, .offset = 0.0f },
	{ .name = "servo_en",            .type = LOG_TYPE_U8,  .unit = LOG_UNIT_BOOL,       .bit_off = 0, .bit_len = 0, .advance = 1, .scale = 1.0f, .offset = 0.0f },
};

#define LOG_FIELD_COUNT  (sizeof(log_fields) / sizeof(log_fields[0]))

static uint8_t schema_blob[LOG_SCHEMA_BLOB_MAX];
static uint16_t schema_blob_len;
static uint16_t schema_blob_crc;
static uint8_t schema_chunk_count;
static uint8_t schema_next_chunk;
static uint16_t schema_frame_size;
static bool schema_built;

_Static_assert(sizeof(log_schema_chunk_hdr_t) == LOG_SCHEMA_HDR_SIZE, "schema chunk header size");
_Static_assert(LOG_SCHEMA_HDR_SIZE + LOG_SCHEMA_SLICE == LOG_SCHEMA_MAX_PAYLOAD, "schema slice size");
_Static_assert(sizeof(DataPackage_t) == 112, "DataPackage_t size");
_Static_assert(LOG_FIELD_COUNT <= LOG_SCHEMA_MAX_FIELDS, "too many log fields");


//----- Private helpers ----------

static uint8_t LogSchema_typeSize(uint8_t type){
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

/* CRC-16/ARC, init 0xFFFF, poly 0xA001. Same algorithm as SimpleFS_crc16. */
static uint16_t LogSchema_crc16(const uint8_t *buf, uint32_t len){
	uint16_t crc = 0xFFFF;

	for(uint32_t i = 0; i < len; i++){
		crc ^= buf[i];
		for(uint8_t bit = 0; bit < 8; bit++){
			if(crc & 1u){
				crc = (uint16_t)((crc >> 1) ^ 0xA001u);
			} else {
				crc = (uint16_t)(crc >> 1);
			}
		}
	}

	return crc;
}

static bool LogSchema_nameOk(const char *name){
	size_t len = strlen(name);
	if(len < 1 || len > LOG_SCHEMA_NAME_MAX){
		return false;
	}
	if(!(isalpha((unsigned char)name[0]) || name[0] == '_')){
		return false;
	}
	for(size_t i = 1; i < len; i++){
		if(!(isalnum((unsigned char)name[i]) || name[i] == '_')){
			return false;
		}
	}
	return true;
}

static esp_err_t LogSchema_append(const void *src, uint16_t len){
	if((uint32_t)schema_blob_len + len > LOG_SCHEMA_BLOB_MAX){
		ESP_LOGE(TAG, "Schema blob full");
		return ESP_FAIL;
	}
	memcpy(&schema_blob[schema_blob_len], src, len);
	schema_blob_len = (uint16_t)(schema_blob_len + len);
	return ESP_OK;
}

static esp_err_t LogSchema_build(void){
	schema_blob_len = 0;

	for(uint16_t i = 0; i < LOG_FIELD_COUNT; i++){
		const log_field_def_t *field = &log_fields[i];
		uint8_t name_len = (uint8_t)strlen(field->name);
		uint8_t tail[5];

		if(LogSchema_append(&name_len, 1) != ESP_OK){
			return ESP_FAIL;
		}
		if(LogSchema_append(field->name, name_len) != ESP_OK){
			return ESP_FAIL;
		}

		tail[0] = field->type;
		tail[1] = field->unit;
		tail[2] = field->bit_off;
		tail[3] = field->bit_len;
		tail[4] = field->advance;
		if(LogSchema_append(tail, sizeof(tail)) != ESP_OK){
			return ESP_FAIL;
		}
		if(LogSchema_append(&field->scale, sizeof(field->scale)) != ESP_OK){
			return ESP_FAIL;
		}
		if(LogSchema_append(&field->offset, sizeof(field->offset)) != ESP_OK){
			return ESP_FAIL;
		}
	}

	if(schema_blob_len == 0){
		ESP_LOGE(TAG, "Schema blob empty");
		return ESP_FAIL;
	}

	schema_blob_crc = LogSchema_crc16(schema_blob, schema_blob_len);
	schema_chunk_count = (uint8_t)((schema_blob_len + LOG_SCHEMA_SLICE - 1u) / LOG_SCHEMA_SLICE);
	if(schema_chunk_count == 0){
		ESP_LOGE(TAG, "Schema chunk count is 0");
		return ESP_FAIL;
	}

	return ESP_OK;
}

static esp_err_t LogSchema_checkNamedOffset(const char *name, uint16_t cursor){
	uint16_t expect = 0;
	bool known = true;

	if(strcmp(name, "sys_time") == 0){
		expect = offsetof(DataPackage_t, sys_time);
	} else if(strcmp(name, "accX") == 0){
		expect = offsetof(DataPackage_t, sensors.accX);
	} else if(strcmp(name, "pressure") == 0){
		expect = offsetof(DataPackage_t, sensors.pressure);
	} else if(strcmp(name, "temp") == 0){
		expect = offsetof(DataPackage_t, sensors.temp);
	} else if(strcmp(name, "latitude") == 0){
		expect = offsetof(DataPackage_t, sensors.latitude);
	} else if(strcmp(name, "tilt") == 0){
		expect = offsetof(DataPackage_t, ahrs.tilt);
	} else if(strcmp(name, "flightstate") == 0){
		expect = offsetof(DataPackage_t, flightstate);
	} else if(strcmp(name, "vbat_mV") == 0){
		expect = offsetof(DataPackage_t, vbat_mV);
	} else if(strcmp(name, "servo_en") == 0){
		expect = offsetof(DataPackage_t, servo.servo_en);
	} else {
		known = false;
	}

	if(known && cursor != expect){
		ESP_LOGE(TAG, "%s at %u, struct offset %u", name, cursor, expect);
		return ESP_FAIL;
	}
	return ESP_OK;
}

static esp_err_t LogSchema_checkAdvance(uint16_t index, uint8_t size){
	const log_field_def_t *field = &log_fields[index];
	uint8_t expect;

	if(field->bit_len == 0){
		expect = size;
	} else {
		bool last = true;
		if((uint32_t)index + 1u < LOG_FIELD_COUNT){
			const log_field_def_t *next = &log_fields[index + 1u];
			if(next->bit_len != 0 && next->bit_off > field->bit_off){
				last = false;
			}
		}
		expect = last ? size : 0;
	}

	if(field->advance != expect){
		ESP_LOGE(TAG, "%s advance %u, expected %u", field->name, field->advance, expect);
		return ESP_FAIL;
	}
	return ESP_OK;
}

static esp_err_t LogSchema_checkTable(void){
	uint16_t cursor = 0;

	for(uint16_t i = 0; i < LOG_FIELD_COUNT; i++){
		const log_field_def_t *field = &log_fields[i];
		uint8_t size = LogSchema_typeSize(field->type);

		if(size == 0){
			ESP_LOGE(TAG, "Bad type for %s", field->name);
			return ESP_FAIL;
		}
		if(!LogSchema_nameOk(field->name)){
			ESP_LOGE(TAG, "Bad field name %s", field->name);
			return ESP_FAIL;
		}
		if(field->bit_len != 0 &&
				((uint16_t)field->bit_off + field->bit_len) > ((uint16_t)size * 8u)){
			ESP_LOGE(TAG, "%s bits do not fit in the type", field->name);
			return ESP_FAIL;
		}
		if(LogSchema_checkNamedOffset(field->name, cursor) != ESP_OK){
			return ESP_FAIL;
		}
		if(LogSchema_checkAdvance(i, size) != ESP_OK){
			return ESP_FAIL;
		}

		cursor = (uint16_t)(cursor + field->advance);
	}

	if(cursor != 108){
		ESP_LOGE(TAG, "Schema cursor ends at %u, expected 108", cursor);
		return ESP_FAIL;
	}
	if(cursor > sizeof(DataPackage_t)){
		ESP_LOGE(TAG, "Schema cursor walks past DataPackage_t");
		return ESP_FAIL;
	}
	return ESP_OK;
}

static esp_err_t LogSchema_checkIgnBits(void){
	DataPackage_t package;
	uint8_t ign_byte;

	memset(&package, 0, sizeof(package));
	package.ign.ign1_cont = 1;
	ign_byte = *(uint8_t *)&package.ign;
	if(ign_byte != 0x01){
		ESP_LOGE(TAG, "ign1_cont is not bit 0 (byte 0x%02X)", ign_byte);
		return ESP_FAIL;
	}

	memset(&package.ign, 0, sizeof(package.ign));
	package.ign.ign4_state = 1;
	ign_byte = *(uint8_t *)&package.ign;
	if(ign_byte != 0x80){
		ESP_LOGE(TAG, "ign4_state is not bit 7 (byte 0x%02X)", ign_byte);
		return ESP_FAIL;
	}
	return ESP_OK;
}

static esp_err_t LogSchema_checkBlob(void){
	uint16_t pos = 0;
	uint16_t cursor = 0;

	for(uint16_t i = 0; i < LOG_FIELD_COUNT; i++){
		const log_field_def_t *field = &log_fields[i];
		uint8_t name_len;
		float scale;
		float offset;

		if(pos >= schema_blob_len){
			ESP_LOGE(TAG, "Blob ended at field %s", field->name);
			return ESP_FAIL;
		}

		name_len = schema_blob[pos++];
		if(name_len != strlen(field->name)){
			ESP_LOGE(TAG, "Blob name length mismatch for %s", field->name);
			return ESP_FAIL;
		}
		if((uint32_t)pos + name_len + 5u + 8u > schema_blob_len){
			ESP_LOGE(TAG, "Blob truncated at %s", field->name);
			return ESP_FAIL;
		}
		if(memcmp(&schema_blob[pos], field->name, name_len) != 0){
			ESP_LOGE(TAG, "Blob name mismatch for %s", field->name);
			return ESP_FAIL;
		}
		pos = (uint16_t)(pos + name_len);

		if(schema_blob[pos] != field->type ||
				schema_blob[pos + 1] != field->unit ||
				schema_blob[pos + 2] != field->bit_off ||
				schema_blob[pos + 3] != field->bit_len ||
				schema_blob[pos + 4] != field->advance){
			ESP_LOGE(TAG, "Blob tail mismatch for %s", field->name);
			return ESP_FAIL;
		}
		pos = (uint16_t)(pos + 5);

		memcpy(&scale, &schema_blob[pos], sizeof(scale));
		pos = (uint16_t)(pos + sizeof(scale));
		memcpy(&offset, &schema_blob[pos], sizeof(offset));
		pos = (uint16_t)(pos + sizeof(offset));
		if(scale != field->scale || offset != field->offset){
			ESP_LOGE(TAG, "Blob scale/offset mismatch for %s", field->name);
			return ESP_FAIL;
		}

		cursor = (uint16_t)(cursor + field->advance);
	}

	if(pos != schema_blob_len){
		ESP_LOGE(TAG, "Blob has %u trailing bytes", (unsigned)(schema_blob_len - pos));
		return ESP_FAIL;
	}
	if(cursor != 108){
		ESP_LOGE(TAG, "Blob cursor ends at %u", cursor);
		return ESP_FAIL;
	}
	if(LogSchema_crc16(schema_blob, schema_blob_len) != schema_blob_crc){
		ESP_LOGE(TAG, "Blob CRC mismatch");
		return ESP_FAIL;
	}
	return ESP_OK;
}

static esp_err_t LogSchema_checkFrames(void){
	uint8_t frame[LOG_SCHEMA_MAX_PAYLOAD];
	uint8_t rebuilt[LOG_SCHEMA_BLOB_MAX];
	uint16_t filled = 0;

	LogSchema_rewind();

	for(uint8_t i = 0; i < schema_chunk_count; i++){
		log_schema_chunk_hdr_t hdr;
		uint16_t slice;
		uint16_t len;

		if(LogSchema_generate(frame, sizeof(frame)) != ESP_OK){
			ESP_LOGE(TAG, "Missing schema chunk %u", i);
			return ESP_FAIL;
		}
		len = LogSchema_size();
		if(len < sizeof(hdr)){
			ESP_LOGE(TAG, "Schema chunk %u is too short", i);
			return ESP_FAIL;
		}

		memcpy(&hdr, frame, sizeof(hdr));
		slice = (uint16_t)(len - sizeof(hdr));

		if(hdr.magic != LOG_SCHEMA_MAGIC ||
				hdr.version != LOG_SCHEMA_VERSION ||
				hdr.flags != LOG_SCHEMA_FLAG_LE ||
				hdr.chunk_index != i ||
				hdr.chunk_count != schema_chunk_count ||
				hdr.field_count != LOG_FIELD_COUNT ||
				hdr.sample_bytes != sizeof(DataPackage_t) ||
				hdr.rate_hz != CONFIG_MAIN_LOOP_FREQUENCY ||
				hdr.blob_bytes != schema_blob_len ||
				hdr.blob_crc != schema_blob_crc){
			ESP_LOGE(TAG, "Schema chunk %u header mismatch", i);
			return ESP_FAIL;
		}
		if((uint32_t)filled + slice > schema_blob_len){
			ESP_LOGE(TAG, "Schema chunk %u overflows the blob", i);
			return ESP_FAIL;
		}

		memcpy(&rebuilt[filled], &frame[sizeof(hdr)], slice);
		filled = (uint16_t)(filled + slice);
	}

	if(LogSchema_generate(frame, sizeof(frame)) != ESP_ERR_NOT_FOUND){
		ESP_LOGE(TAG, "Schema produced an extra chunk");
		return ESP_FAIL;
	}
	if(filled != schema_blob_len || memcmp(rebuilt, schema_blob, filled) != 0){
		ESP_LOGE(TAG, "Schema frames do not rebuild the blob");
		return ESP_FAIL;
	}

	LogSchema_rewind();
	return ESP_OK;
}


//----- Public API ----------

esp_err_t LogSchema_init(void){
	schema_built = false;
	schema_next_chunk = 0;

	if(LogSchema_build() != ESP_OK){
		return ESP_FAIL;
	}

	schema_built = true;
	if(LogSchema_selfCheck() != ESP_OK){
		schema_built = false;
		return ESP_FAIL;
	}

	LogSchema_rewind();
	ESP_LOGI(TAG, "Schema ready: %u fields, %u bytes, %u frames",
			(unsigned)LOG_FIELD_COUNT, schema_blob_len, schema_chunk_count);
	return ESP_OK;
}

esp_err_t LogSchema_selfCheck(void){
	if(LogSchema_checkTable() != ESP_OK){
		return ESP_FAIL;
	}
	if(LogSchema_checkIgnBits() != ESP_OK){
		return ESP_FAIL;
	}
	if(!schema_built){
		ESP_LOGE(TAG, "Schema blob is not built");
		return ESP_FAIL;
	}
	if(LogSchema_checkBlob() != ESP_OK){
		return ESP_FAIL;
	}
	if(LogSchema_checkFrames() != ESP_OK){
		return ESP_FAIL;
	}
	return ESP_OK;
}

void LogSchema_rewind(void){
	schema_next_chunk = 0;
}

uint16_t LogSchema_size(void){
	return schema_frame_size;
}

esp_err_t LogSchema_generate(uint8_t *buf, uint16_t buf_size){
	static bool schema_init_failed = false;
	uint16_t offset;
	uint16_t remain;
	uint16_t slice;
	uint16_t total;
	log_schema_chunk_hdr_t hdr;

	if(buf == NULL){
		return ESP_ERR_INVALID_ARG;
	}
	if(schema_init_failed){
		return ESP_FAIL;
	}
	/* Self-check calls this while init is already in progress and schema_built is set. */
	if(!schema_built){
		if(LogSchema_init() != ESP_OK){
			schema_init_failed = true;
			return ESP_FAIL;
		}
	}
	if(schema_next_chunk >= schema_chunk_count){
		return ESP_ERR_NOT_FOUND;
	}

	offset = (uint16_t)(schema_next_chunk * LOG_SCHEMA_SLICE);
	remain = (uint16_t)(schema_blob_len - offset);
	slice = remain > LOG_SCHEMA_SLICE ? LOG_SCHEMA_SLICE : remain;
	total = (uint16_t)(sizeof(hdr) + slice);
	if(buf_size < total){
		return ESP_ERR_INVALID_ARG;
	}

	memset(&hdr, 0, sizeof(hdr));
	hdr.magic = LOG_SCHEMA_MAGIC;
	hdr.version = LOG_SCHEMA_VERSION;
	hdr.chunk_index = schema_next_chunk;
	hdr.chunk_count = schema_chunk_count;
	hdr.flags = LOG_SCHEMA_FLAG_LE;
	hdr.field_count = (uint16_t)LOG_FIELD_COUNT;
	hdr.sample_bytes = (uint16_t)sizeof(DataPackage_t);
	hdr.rate_hz = CONFIG_MAIN_LOOP_FREQUENCY;
	hdr.blob_bytes = schema_blob_len;
	hdr.blob_crc = schema_blob_crc;

	memcpy(buf, &hdr, sizeof(hdr));
	memcpy(buf + sizeof(hdr), &schema_blob[offset], slice);
	schema_frame_size = total;
	schema_next_chunk++;
	return ESP_OK;
}
