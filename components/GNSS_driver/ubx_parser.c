#include <string.h>
#include "ubx_parser.h"

enum {
	ST_SYNC1 = 0,
	ST_SYNC2,
	ST_CLASS,
	ST_ID,
	ST_LEN1,
	ST_LEN2,
	ST_PAYLOAD,
	ST_CK_A,
	ST_CK_B
};

static inline void ck_add(ubx_parser_t *p, uint8_t b)
{
	p->ck_a = (uint8_t)(p->ck_a + b);
	p->ck_b = (uint8_t)(p->ck_b + p->ck_a);
}

void ubx_parser_init(ubx_parser_t *p, ubx_frame_cb_t cb, void *ctx)
{
	memset(p, 0, sizeof(*p));
	p->state = ST_SYNC1;
	p->cb = cb;
	p->ctx = ctx;
}

void ubx_parser_feed(ubx_parser_t *p, const uint8_t *data, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		uint8_t b = data[i];

		switch (p->state) {
		case ST_SYNC1:
			if (b == UBX_SYNC1)
				p->state = ST_SYNC2;
			break;

		case ST_SYNC2:
			if (b == UBX_SYNC2) {
				p->state = ST_CLASS;
				p->ck_a = 0;
				p->ck_b = 0;
			} else if (b != UBX_SYNC1) {
				p->state = ST_SYNC1;   // a repeated 0xB5 may still be the real sync
			}
			break;

		case ST_CLASS:
			p->cls = b;
			ck_add(p, b);
			p->state = ST_ID;
			break;

		case ST_ID:
			p->id = b;
			ck_add(p, b);
			p->state = ST_LEN1;
			break;

		case ST_LEN1:
			p->len = b;
			ck_add(p, b);
			p->state = ST_LEN2;
			break;

		case ST_LEN2:
			p->len |= (uint16_t)((uint16_t)b << 8);
			ck_add(p, b);
			p->pos = 0;
			p->state = (p->len > 0) ? ST_PAYLOAD : ST_CK_A;
			break;

		case ST_PAYLOAD:
			if (p->pos < UBX_PAYLOAD_MAX)
				p->payload[p->pos] = b;
			p->pos++;
			ck_add(p, b);
			if (p->pos >= p->len)
				p->state = ST_CK_A;
			break;

		case ST_CK_A:
			p->rx_ck_a = b;
			p->state = ST_CK_B;
			break;

		case ST_CK_B:
			if (p->rx_ck_a == p->ck_a && b == p->ck_b) {
				p->frames_ok++;
				if (p->cb) {
					uint16_t stored = (p->len < UBX_PAYLOAD_MAX) ? p->len : UBX_PAYLOAD_MAX;
					p->cb(p->cls, p->id, p->payload, stored, p->ctx);
				}
			} else {
				p->frames_bad++;
			}
			p->state = ST_SYNC1;
			break;

		default:
			p->state = ST_SYNC1;
			break;
		}
	}
}

size_t ubx_build_frame(uint8_t cls, uint8_t id, const uint8_t *payload, uint16_t len, uint8_t *out, size_t out_size)
{
	if (out_size < (size_t)len + UBX_FRAME_OVERHEAD)
		return 0;

	out[0] = UBX_SYNC1;
	out[1] = UBX_SYNC2;
	out[2] = cls;
	out[3] = id;
	out[4] = (uint8_t)(len & 0xFF);
	out[5] = (uint8_t)(len >> 8);
	if (len)
		memcpy(&out[6], payload, len);

	uint8_t a = 0, b = 0;
	for (size_t i = 2; i < (size_t)len + 6; i++) {
		a = (uint8_t)(a + out[i]);
		b = (uint8_t)(b + a);
	}
	out[6 + len] = a;
	out[7 + len] = b;
	return (size_t)len + UBX_FRAME_OVERHEAD;
}

uint8_t ubx_key_value_size(uint32_t key)
{
	switch ((key >> 28) & 0x7) {
	case 1: return 1;  // L  (1 bit, stored in 1 byte)
	case 2: return 1;  // U1/I1/E1/X1
	case 3: return 2;  // U2/I2/E2/X2
	case 4: return 4;  // U4/I4/E4/X4/R4
	case 5: return 8;  // U8/I8/R8/X8
	default: return 0;
	}
}

size_t ubx_build_valset(uint8_t layers, const ubx_cfg_item_t *items, size_t n_items, uint8_t *out, size_t out_size)
{
	uint8_t payload[UBX_PAYLOAD_MAX];
	size_t pos = 0;

	payload[pos++] = 0x00;     // version
	payload[pos++] = layers;
	payload[pos++] = 0x00;     // reserved
	payload[pos++] = 0x00;

	for (size_t i = 0; i < n_items; i++) {
		uint8_t sz = ubx_key_value_size(items[i].key);
		if (sz == 0 || pos + 4 + sz > sizeof(payload))
			return 0;
		for (int k = 0; k < 4; k++)
			payload[pos++] = (uint8_t)(items[i].key >> (8 * k));
		for (uint8_t k = 0; k < sz; k++)
			payload[pos++] = (uint8_t)(items[i].value >> (8 * k));
	}
	return ubx_build_frame(UBX_CLASS_CFG, UBX_ID_CFG_VALSET, payload, (uint16_t)pos, out, out_size);
}

static inline uint16_t rd_u16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static inline uint32_t rd_u32(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }
static inline int32_t  rd_i32(const uint8_t *p) { return (int32_t)rd_u32(p); }

bool ubx_decode_nav_pvt(const uint8_t *p, uint16_t len, ubx_nav_pvt_t *o)
{
	if (len < UBX_NAV_PVT_LEN)
		return false;

	o->itow     = rd_u32(p + 0);
	o->year     = rd_u16(p + 4);
	o->month    = p[6];
	o->day      = p[7];
	o->hour     = p[8];
	o->min      = p[9];
	o->sec      = p[10];
	o->valid    = p[11];
	o->nano     = rd_i32(p + 16);
	o->fix_type = p[20];
	o->flags    = p[21];
	o->num_sv   = p[23];
	o->lon      = rd_i32(p + 24);
	o->lat      = rd_i32(p + 28);
	o->height   = rd_i32(p + 32);
	o->hmsl     = rd_i32(p + 36);
	o->h_acc    = rd_u32(p + 40);
	o->v_acc    = rd_u32(p + 44);
	o->vel_n    = rd_i32(p + 48);
	o->vel_e    = rd_i32(p + 52);
	o->vel_d    = rd_i32(p + 56);
	o->g_speed  = rd_i32(p + 60);
	o->head_mot = rd_i32(p + 64);
	o->s_acc    = rd_u32(p + 68);
	o->p_dop    = rd_u16(p + 76);
	o->mag_dec  = (int16_t)rd_u16(p + 88);
	return true;
}

bool ubx_decode_nav_dop(const uint8_t *p, uint16_t len, ubx_nav_dop_t *o)
{
	if (len < UBX_NAV_DOP_LEN)
		return false;

	o->itow  = rd_u32(p + 0);
	o->g_dop = rd_u16(p + 4)  * 0.01f;
	o->p_dop = rd_u16(p + 6)  * 0.01f;
	o->t_dop = rd_u16(p + 8)  * 0.01f;
	o->v_dop = rd_u16(p + 10) * 0.01f;
	o->h_dop = rd_u16(p + 12) * 0.01f;
	o->n_dop = rd_u16(p + 14) * 0.01f;
	o->e_dop = rd_u16(p + 16) * 0.01f;
	return true;
}

void ubx_pvt_to_gps(const ubx_nav_pvt_t *pvt, gps_t *gps)
{
	bool fix_ok = (pvt->flags & 0x01) != 0;                   // gnssFixOK
	bool diff   = (pvt->flags & 0x02) != 0;                   // diffSoln
	bool has_fix = fix_ok && (pvt->fix_type == 2 || pvt->fix_type == 3 || pvt->fix_type == 4);

	gps->valid = has_fix;
	gps->fix   = !has_fix ? GPS_FIX_INVALID : (diff ? GPS_FIX_DGPS : GPS_FIX_GPS);
	if (!has_fix)
		gps->fix_mode = GPS_MODE_INVALID;
	else
		gps->fix_mode = (pvt->fix_type == 2) ? GPS_MODE_2D : GPS_MODE_3D;

	gps->sats_in_use = pvt->num_sv;
	gps->itow_ms = pvt->itow;

	gps->latitude  = pvt->lat * 1e-7;
	gps->longitude = pvt->lon * 1e-7;
	gps->altitude           = pvt->hmsl   * 1e-3f;
	gps->altitude_ellipsoid = pvt->height * 1e-3f;

	gps->tim.hour   = pvt->hour;
	gps->tim.minute = pvt->min;
	gps->tim.second = pvt->sec;
	gps->tim.thousand = (pvt->nano > 0) ? (uint16_t)((pvt->nano / 1000000) % 1000) : 0;
	gps->date.day   = pvt->day;
	gps->date.month = pvt->month;
	gps->date.year  = pvt->year;

	gps->speed = pvt->g_speed * 1e-3f;
	gps->cog   = pvt->head_mot * 1e-5f;
	gps->variation = pvt->mag_dec * 1e-2f;
	gps->vel_n = pvt->vel_n * 1e-3f;
	gps->vel_e = pvt->vel_e * 1e-3f;
	gps->vel_d = pvt->vel_d * 1e-3f;

	gps->h_acc = pvt->h_acc * 1e-3f;
	gps->v_acc = pvt->v_acc * 1e-3f;
	gps->s_acc = pvt->s_acc * 1e-3f;
	gps->dop_p = pvt->p_dop * 0.01f;

	gps->source = GNSS_TYPE_UBLOX_UBX;
}

uint8_t ubx_dynmodel_from_nav_mode(gps_nav_mode_t mode)
{
	switch (mode) {
	case GPS_MODE_FITNESS:    return 3;  // pedestrian
	case GPS_MODE_AVIATION:   return 8;  // airborne, <4g
	case GPS_MODE_BALLON:     return 6;  // airborne, <1g
	case GPS_MODE_STATIONARY: return 2;  // stationary
	case GPS_MODE_NORMAL:
	default:                  return 0;  // portable
	}
}
