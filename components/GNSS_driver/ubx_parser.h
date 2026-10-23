#pragma once

/**
 * @file ubx_parser.h
 * @brief u-blox UBX protocol framing, message decoding and frame building.
 *
 * Pure C, no ESP-IDF dependencies (host-testable, see host_test/).
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "GNSS_types.h"

#define UBX_SYNC1 0xB5
#define UBX_SYNC2 0x62

/* Message classes / ids used by the driver */
#define UBX_CLASS_NAV 0x01
#define UBX_ID_NAV_PVT 0x07
#define UBX_ID_NAV_DOP 0x04
#define UBX_CLASS_ACK 0x05
#define UBX_ID_ACK_NAK 0x00
#define UBX_ID_ACK_ACK 0x01
#define UBX_CLASS_CFG 0x06
#define UBX_ID_CFG_VALSET 0x8A
#define UBX_CLASS_MON 0x0A
#define UBX_ID_MON_VER 0x04

#define UBX_NAV_PVT_LEN 92
#define UBX_NAV_DOP_LEN 18

/** Largest payload kept by the parser. Longer frames are still checksummed but delivered truncated. */
#define UBX_PAYLOAD_MAX 128
/** Frame overhead: 2 sync + class + id + 2 length + 2 checksum */
#define UBX_FRAME_OVERHEAD 8

/* VALSET layers */
#define UBX_LAYER_RAM   0x01
#define UBX_LAYER_BBR   0x02
#define UBX_LAYER_FLASH 0x04

/* Configuration keys (u-blox M10 interface description) */
#define UBX_KEY_UART1_BAUDRATE          0x40520001u
#define UBX_KEY_UART1INPROT_UBX         0x10730001u
#define UBX_KEY_UART1INPROT_NMEA        0x10730002u
#define UBX_KEY_UART1OUTPROT_UBX        0x10740001u
#define UBX_KEY_UART1OUTPROT_NMEA       0x10740002u
#define UBX_KEY_RATE_MEAS               0x30210001u
#define UBX_KEY_RATE_NAV                0x30210002u
#define UBX_KEY_NAVSPG_DYNMODEL         0x20110021u
#define UBX_KEY_MSGOUT_NAV_PVT_UART1    0x20910007u
#define UBX_KEY_MSGOUT_NAV_DOP_UART1    0x20910039u
#define UBX_KEY_SIGNAL_GPS_ENA          0x1031001fu
#define UBX_KEY_SIGNAL_SBAS_ENA         0x10310020u
#define UBX_KEY_SIGNAL_GAL_ENA          0x10310021u
#define UBX_KEY_SIGNAL_BDS_ENA          0x10310022u
#define UBX_KEY_SIGNAL_QZSS_ENA         0x10310024u
#define UBX_KEY_SIGNAL_GLO_ENA          0x10310025u

/** UBX-CFG-VALSET key/value pair. Value width is derived from the key (bits 28..30). */
typedef struct {
	uint32_t key;
	uint64_t value;
} ubx_cfg_item_t;

/** Parser frame callback; payload is valid only during the call. */
typedef void (*ubx_frame_cb_t)(uint8_t cls, uint8_t id, const uint8_t *payload, uint16_t len, void *ctx);

typedef struct {
	uint8_t  state;
	uint8_t  cls;
	uint8_t  id;
	uint16_t len;       /*!< payload length announced by the frame */
	uint16_t pos;       /*!< payload bytes consumed so far */
	uint8_t  ck_a;
	uint8_t  ck_b;
	uint8_t  rx_ck_a;
	uint8_t  payload[UBX_PAYLOAD_MAX];
	uint32_t frames_ok;
	uint32_t frames_bad;  /*!< checksum errors */
	ubx_frame_cb_t cb;
	void *ctx;
} ubx_parser_t;

/** NAV-PVT in raw receiver units */
typedef struct {
	uint32_t itow;
	uint16_t year;
	uint8_t month, day, hour, min, sec;
	uint8_t valid;
	int32_t nano;
	uint8_t fix_type;
	uint8_t flags;
	uint8_t num_sv;
	int32_t lon, lat;         /*!< 1e-7 deg */
	int32_t height, hmsl;     /*!< mm */
	uint32_t h_acc, v_acc;    /*!< mm */
	int32_t vel_n, vel_e, vel_d, g_speed; /*!< mm/s */
	int32_t head_mot;         /*!< 1e-5 deg */
	uint32_t s_acc;           /*!< mm/s */
	uint16_t p_dop;           /*!< 0.01 */
	int16_t mag_dec;          /*!< 0.01 deg */
} ubx_nav_pvt_t;

/** NAV-DOP, values already scaled (x0.01 applied) */
typedef struct {
	uint32_t itow;
	float g_dop, p_dop, t_dop, v_dop, h_dop, n_dop, e_dop;
} ubx_nav_dop_t;

void ubx_parser_init(ubx_parser_t *p, ubx_frame_cb_t cb, void *ctx);
/** Feed raw bytes from the receiver. Resynchronises automatically on errors. */
void ubx_parser_feed(ubx_parser_t *p, const uint8_t *data, size_t len);

/** Build a complete frame (sync, header, payload, checksum). Returns frame size or 0 if out is too small. */
size_t ubx_build_frame(uint8_t cls, uint8_t id, const uint8_t *payload, uint16_t len, uint8_t *out, size_t out_size);
/** Build UBX-CFG-VALSET frame. Returns frame size or 0 on error. */
size_t ubx_build_valset(uint8_t layers, const ubx_cfg_item_t *items, size_t n_items, uint8_t *out, size_t out_size);
/** Size in bytes of the value belonging to a configuration key, 0 if the key encodes an unknown size. */
uint8_t ubx_key_value_size(uint32_t key);

bool ubx_decode_nav_pvt(const uint8_t *payload, uint16_t len, ubx_nav_pvt_t *out);
bool ubx_decode_nav_dop(const uint8_t *payload, uint16_t len, ubx_nav_dop_t *out);

/** Convert a decoded NAV-PVT into gps_t. Fields not provided by PVT (dop_h, dop_v, rx_time_us) are left untouched. */
void ubx_pvt_to_gps(const ubx_nav_pvt_t *pvt, gps_t *gps);

/** Map navigation mode to the u-blox dynamic platform model (CFG-NAVSPG-DYNMODEL). */
uint8_t ubx_dynmodel_from_nav_mode(gps_nav_mode_t mode);
