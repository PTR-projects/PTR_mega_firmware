#pragma once

#include <stdint.h>
#include "esp_err.h"

/**
 * @brief Self-describing header for the measurement log.
 *
 * A logging session starts with one or more SimpleFS frames whose preamble is
 * SFS_HEADER_SCHEMA (0xAA56). Each frame carries a slice of a descriptor blob.
 * Sample frames stay preamble 0xAA55 and still hold one raw DataPackage_t.
 *
 * A field is stored as: name length, name bytes, type, unit, bit offset,
 * bit length, advance, float scale, float offset. All multi-byte values are
 * little-endian. The decoded value is meas * scale - offset. Offset is in
 * engineering units, applied after scale.
 *
 * bit_len 0 means the whole value. Otherwise meas is the unsigned bit field
 * of bit_len bits starting at bit_off. advance is how many bytes to move the
 * cursor afterwards. Zero keeps the cursor on the same word so several names
 * can share one byte (GNSS fix packing, igniter flags).
 *
 * Changing DataPackage_t means editing the field table in LogSchema.c. The
 * web and SD parsers read this header from the log; they do not need a
 * matching table for new files. A file with no schema is still the legacy
 * fixed layout.
 */

#define LOG_SCHEMA_MAGIC        0x314C5450u  /*!< Bytes 'P''T''L''1' on the wire. */
#define LOG_SCHEMA_VERSION      1u
#define LOG_SCHEMA_FLAG_LE      0x01u        /*!< flags value for little-endian. */
#define LOG_SCHEMA_NAME_MAX     19u
#define LOG_SCHEMA_MAX_FIELDS   64u
#define LOG_SCHEMA_BLOB_MAX     2048u
/* Must match the SimpleFS frame payload (sfs_packet_t.payload). */
#define LOG_SCHEMA_MAX_PAYLOAD  122u
#define LOG_SCHEMA_HDR_SIZE     18u
#define LOG_SCHEMA_SLICE        (LOG_SCHEMA_MAX_PAYLOAD - LOG_SCHEMA_HDR_SIZE)

/**
 * @brief Number type stored in a sample, before scale and offset.
 */
typedef enum {
	LOG_TYPE_I8  = 1,
	LOG_TYPE_U8  = 2,
	LOG_TYPE_I16 = 3,
	LOG_TYPE_U16 = 4,
	LOG_TYPE_I32 = 5,
	LOG_TYPE_U32 = 6,
	LOG_TYPE_F16 = 7,
	LOG_TYPE_F32 = 8,
	LOG_TYPE_F64 = 9,
} log_type_t;

/**
 * @brief Engineering unit of meas * scale - offset.
 * The CSV columns use the field name. The unit byte is there for a later tool.
 */
typedef enum {
	LOG_UNIT_NONE = 0,
	LOG_UNIT_RAW,
	LOG_UNIT_BOOL,
	LOG_UNIT_COUNT,
	LOG_UNIT_TICK_0_1MS,
	LOG_UNIT_G,
	LOG_UNIT_DPS,
	LOG_UNIT_DEG,
	LOG_UNIT_M,
	LOG_UNIT_MPS,
	LOG_UNIT_PA,
	LOG_UNIT_DEGC,
	LOG_UNIT_MV,
	LOG_UNIT_PERCENT,
} log_unit_t;

/**
 * @brief Fixed header at the start of every schema frame payload.
 * The descriptor slice follows immediately. A short last slice is padded
 * with zeros up to the SimpleFS payload; readers take
 * min(LOG_SCHEMA_SLICE, blob_bytes - chunk_index * LOG_SCHEMA_SLICE) bytes.
 */
typedef struct __attribute__((__packed__)) {
	uint32_t magic;
	uint8_t version;
	uint8_t chunk_index;
	uint8_t chunk_count;
	uint8_t flags;
	uint16_t field_count;
	uint16_t sample_bytes;
	uint16_t rate_hz;
	uint16_t blob_bytes;
	uint16_t blob_crc;
} log_schema_chunk_hdr_t;

/**
 * @brief Build the schema blob and check it against DataPackage_t.
 * @return ESP_OK when the table, the blob, and the frame split agree.
 */
esp_err_t LogSchema_init(void);

/**
 * @brief Check field offsets, bit order, and the serialized blob.
 * Called from LogSchema_init().
 */
esp_err_t LogSchema_selfCheck(void);

/**
 * @brief Start the schema frames again from the first chunk.
 * Used when a logging session begins, and when a frame write has to be retried.
 */
void LogSchema_rewind(void);

/**
 * @brief Copy the next schema frame payload into buf.
 * @param buf      Destination, at least LOG_SCHEMA_MAX_PAYLOAD bytes.
 * @param buf_size Size of buf.
 * @param len      Set to the payload length (header plus slice, not zero padding).
 * @return ESP_OK when a frame was copied.
 * @return ESP_ERR_NOT_FOUND when every chunk has already been copied.
 * @return ESP_FAIL if LogSchema_init() did not succeed.
 */
esp_err_t LogSchema_write(uint8_t *buf, uint16_t buf_size, uint16_t *len);
