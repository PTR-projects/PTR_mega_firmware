#include <stdio.h>
#include <string.h>
#include <math.h>
#include "ubx_parser.h"

static int failures, checks;
#define CHECK(c) do { checks++; if (!(c)) { failures++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)
#define CHECK_NEAR(a, b, eps) CHECK(fabs((double)(a) - (double)(b)) <= (eps))

typedef struct { int n; uint8_t cls[16], id[16]; uint16_t len[16]; uint8_t first[16]; } rec_t;

static void on_frame(uint8_t cls, uint8_t id, const uint8_t *p, uint16_t len, void *ctx)
{
	rec_t *r = ctx;
	if (r->n < 16) {
		r->cls[r->n] = cls; r->id[r->n] = id; r->len[r->n] = len; r->first[r->n] = len ? p[0] : 0;
	}
	r->n++;
}

static void put_u32(uint8_t *p, uint32_t v) { for (int i = 0; i < 4; i++) p[i] = (uint8_t)(v >> (8 * i)); }
static void put_u16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }

static size_t make_pvt(uint8_t *frame, size_t sz, int fix_type, uint8_t flags)
{
	uint8_t pl[UBX_NAV_PVT_LEN] = {0};
	put_u32(pl + 0, 345600000);          // iTOW
	put_u16(pl + 4, 2026); pl[6] = 10; pl[7] = 23; pl[8] = 14; pl[9] = 5; pl[10] = 9;
	pl[11] = 0x07;
	put_u32(pl + 16, 250000000);         // nano 250 ms
	pl[20] = (uint8_t)fix_type; pl[21] = flags; pl[23] = 17;
	put_u32(pl + 24, (uint32_t)212345678);    // lon 21.2345678
	put_u32(pl + 28, (uint32_t)521234567);    // lat 52.1234567
	put_u32(pl + 32, (uint32_t)(int32_t)150500);   // height 150.5 m
	put_u32(pl + 36, (uint32_t)(int32_t)110250);   // hMSL 110.25 m
	put_u32(pl + 40, 1500); put_u32(pl + 44, 2500);
	put_u32(pl + 48, (uint32_t)(int32_t)1000);     // velN 1 m/s
	put_u32(pl + 52, (uint32_t)(int32_t)-2000);    // velE -2 m/s
	put_u32(pl + 56, (uint32_t)(int32_t)-30000);   // velD -30 m/s (climbing)
	put_u32(pl + 60, 2236);                        // gSpeed
	put_u32(pl + 64, (uint32_t)(int32_t)9000000);  // headMot 90 deg
	put_u32(pl + 68, 300);
	put_u16(pl + 76, 123);                         // pDOP 1.23
	put_u16(pl + 88, (uint16_t)(int16_t)-150);     // magDec -1.5 deg
	return ubx_build_frame(UBX_CLASS_NAV, UBX_ID_NAV_PVT, pl, sizeof(pl), frame, sz);
}

static void test_known_frames(void)
{
	// UBX-MON-VER poll, documented frame
	uint8_t out[16];
	size_t n = ubx_build_frame(UBX_CLASS_MON, UBX_ID_MON_VER, NULL, 0, out, sizeof(out));
	const uint8_t ver[] = {0xB5, 0x62, 0x0A, 0x04, 0x00, 0x00, 0x0E, 0x34};
	CHECK(n == sizeof(ver) && memcmp(out, ver, n) == 0);

	// UBX-ACK-ACK for CFG-VALSET (0x06 0x8A), documented frame
	const uint8_t ack[] = {0xB5, 0x62, 0x05, 0x01, 0x02, 0x00, 0x06, 0x8A, 0x98, 0xC1};
	rec_t r = {0};
	ubx_parser_t p;
	ubx_parser_init(&p, on_frame, &r);
	ubx_parser_feed(&p, ack, sizeof(ack));
	CHECK(r.n == 1 && r.cls[0] == UBX_CLASS_ACK && r.id[0] == UBX_ID_ACK_ACK && r.len[0] == 2 && r.first[0] == 0x06);
}

static void test_pvt(void)
{
	uint8_t f[128];
	size_t n = make_pvt(f, sizeof(f), 3, 0x01);
	CHECK(n == UBX_NAV_PVT_LEN + UBX_FRAME_OVERHEAD);

	// feed one byte at a time (frame split across reads)
	struct { ubx_nav_pvt_t pvt; int got; } s = {0};
	rec_t r = {0};
	ubx_parser_t p;
	ubx_parser_init(&p, on_frame, &r);
	for (size_t i = 0; i < n; i++) ubx_parser_feed(&p, &f[i], 1);
	CHECK(r.n == 1 && r.cls[0] == UBX_CLASS_NAV && r.id[0] == UBX_ID_NAV_PVT && r.len[0] == UBX_NAV_PVT_LEN);

	CHECK(ubx_decode_nav_pvt(&f[6], UBX_NAV_PVT_LEN, &s.pvt));
	gps_t g; memset(&g, 0, sizeof(g));
	ubx_pvt_to_gps(&s.pvt, &g);
	CHECK(g.fix == GPS_FIX_GPS && g.fix_mode == GPS_MODE_3D && g.valid);
	CHECK(g.sats_in_use == 17);
	CHECK_NEAR(g.latitude, 52.1234567, 1e-9);
	CHECK_NEAR(g.longitude, 21.2345678, 1e-9);
	CHECK_NEAR(g.altitude, 110.25, 1e-3);            // MSL
	CHECK_NEAR(g.altitude_ellipsoid, 150.5, 1e-3);
	CHECK(g.tim.hour == 14 && g.tim.minute == 5 && g.tim.second == 9 && g.tim.thousand == 250);
	CHECK(g.date.year == 2026 && g.date.month == 10 && g.date.day == 23);
	CHECK_NEAR(g.speed, 2.236, 1e-4);
	CHECK_NEAR(g.vel_n, 1.0, 1e-4); CHECK_NEAR(g.vel_e, -2.0, 1e-4); CHECK_NEAR(g.vel_d, -30.0, 1e-4);
	CHECK_NEAR(g.cog, 90.0, 1e-3);
	CHECK_NEAR(g.h_acc, 1.5, 1e-4); CHECK_NEAR(g.v_acc, 2.5, 1e-4); CHECK_NEAR(g.s_acc, 0.3, 1e-4);
	CHECK_NEAR(g.dop_p, 1.23, 1e-4);
	CHECK_NEAR(g.variation, -1.5, 1e-4);
	CHECK(g.itow_ms == 345600000 && g.source == GNSS_TYPE_UBLOX_UBX);

	// differential, 2D, no fix, fix flag not ok
	make_pvt(f, sizeof(f), 2, 0x03);
	ubx_decode_nav_pvt(&f[6], UBX_NAV_PVT_LEN, &s.pvt); ubx_pvt_to_gps(&s.pvt, &g);
	CHECK(g.fix == GPS_FIX_DGPS && g.fix_mode == GPS_MODE_2D);
	make_pvt(f, sizeof(f), 0, 0x00);
	ubx_decode_nav_pvt(&f[6], UBX_NAV_PVT_LEN, &s.pvt); ubx_pvt_to_gps(&s.pvt, &g);
	CHECK(g.fix == GPS_FIX_INVALID && !g.valid && g.fix_mode == GPS_MODE_INVALID);
	make_pvt(f, sizeof(f), 3, 0x00);   // fixType 3 but gnssFixOK = 0
	ubx_decode_nav_pvt(&f[6], UBX_NAV_PVT_LEN, &s.pvt); ubx_pvt_to_gps(&s.pvt, &g);
	CHECK(g.fix == GPS_FIX_INVALID && !g.valid);

	CHECK(!ubx_decode_nav_pvt(f, UBX_NAV_PVT_LEN - 1, &s.pvt));
}

static void test_resync_and_errors(void)
{
	uint8_t a[128], b[128], stream[400];
	size_t na = make_pvt(a, sizeof(a), 3, 0x01);
	size_t nb = ubx_build_frame(UBX_CLASS_ACK, UBX_ID_ACK_NAK, (const uint8_t[]){0x06, 0x8A}, 2, b, sizeof(b));
	size_t pos = 0;

	// junk (incl. NMEA text and a stray sync byte), good, corrupted checksum, junk, good
	const char *junk = "$GNGGA,1,2,3*00\r\n\xB5\x01\x02";
	memcpy(stream + pos, junk, strlen(junk)); pos += strlen(junk);
	memcpy(stream + pos, a, na); pos += na;
	uint8_t bad[128]; memcpy(bad, a, na); bad[20] ^= 0xFF;
	memcpy(stream + pos, bad, na); pos += na - 0;   // corrupted payload byte -> checksum error
	stream[pos++] = 0x00; stream[pos++] = 0xB5;
	memcpy(stream + pos, b, nb); pos += nb;

	rec_t r = {0};
	ubx_parser_t p;
	ubx_parser_init(&p, on_frame, &r);
	ubx_parser_feed(&p, stream, pos);
	CHECK(r.n == 2);
	CHECK(r.id[0] == UBX_ID_NAV_PVT && r.id[1] == UBX_ID_ACK_NAK);
	CHECK(p.frames_bad == 1);
}

static void test_long_frame_truncated(void)
{
	// MON-VER like frame longer than UBX_PAYLOAD_MAX: delivered truncated, checksum still verified
	uint8_t pl[200], f[220];
	for (size_t i = 0; i < sizeof(pl); i++) pl[i] = (uint8_t)(i + 1);
	size_t n = ubx_build_frame(UBX_CLASS_MON, UBX_ID_MON_VER, pl, sizeof(pl), f, sizeof(f));
	rec_t r = {0};
	ubx_parser_t p;
	ubx_parser_init(&p, on_frame, &r);
	ubx_parser_feed(&p, f, n);
	CHECK(r.n == 1 && r.len[0] == UBX_PAYLOAD_MAX && r.first[0] == 1);
	CHECK(ubx_build_frame(UBX_CLASS_MON, UBX_ID_MON_VER, pl, sizeof(pl), f, 100) == 0);
}

static void test_dop(void)
{
	uint8_t pl[UBX_NAV_DOP_LEN] = {0};
	put_u32(pl, 1000);
	put_u16(pl + 6, 150); put_u16(pl + 10, 210); put_u16(pl + 12, 95);
	ubx_nav_dop_t d;
	CHECK(ubx_decode_nav_dop(pl, sizeof(pl), &d));
	CHECK_NEAR(d.p_dop, 1.5, 1e-4); CHECK_NEAR(d.v_dop, 2.1, 1e-4); CHECK_NEAR(d.h_dop, 0.95, 1e-4);
	CHECK(!ubx_decode_nav_dop(pl, 17, &d));
}

static void test_valset(void)
{
	uint8_t f[64];
	ubx_cfg_item_t items[] = {
		{ UBX_KEY_RATE_MEAS, 50 },                  // U2
		{ UBX_KEY_UART1_BAUDRATE, 115200 },         // U4
		{ UBX_KEY_MSGOUT_NAV_PVT_UART1, 1 },        // U1
		{ UBX_KEY_UART1OUTPROT_NMEA, 0 },           // L
	};
	size_t n = ubx_build_valset(UBX_LAYER_RAM, items, 4, f, sizeof(f));
	// header(6) + 4 (ver,layers,res) + (4+2) + (4+4) + (4+1) + (4+1) + 2 checksum
	CHECK(n == 6 + 4 + 6 + 8 + 5 + 5 + 2);
	CHECK(f[2] == UBX_CLASS_CFG && f[3] == UBX_ID_CFG_VALSET);
	CHECK(f[4] == (n - 8) && f[5] == 0);
	CHECK(f[6] == 0 && f[7] == UBX_LAYER_RAM);
	// first item: key 0x30210001 little endian, value 50
	CHECK(f[10] == 0x01 && f[11] == 0x00 && f[12] == 0x21 && f[13] == 0x30 && f[14] == 50 && f[15] == 0);
	// round trip through the parser (checksum validity)
	rec_t r = {0}; ubx_parser_t p; ubx_parser_init(&p, on_frame, &r);
	ubx_parser_feed(&p, f, n);
	CHECK(r.n == 1 && p.frames_bad == 0);
	// invalid key size / too small buffer
	ubx_cfg_item_t bad = { 0x00000001u, 1 };
	CHECK(ubx_build_valset(UBX_LAYER_RAM, &bad, 1, f, sizeof(f)) == 0);
	CHECK(ubx_build_valset(UBX_LAYER_RAM, items, 4, f, 10) == 0);
}

static void test_dynmodel(void)
{
	CHECK(ubx_dynmodel_from_nav_mode(GPS_MODE_AVIATION) == 8);
	CHECK(ubx_dynmodel_from_nav_mode(GPS_MODE_BALLON) == 6);
	CHECK(ubx_dynmodel_from_nav_mode(GPS_MODE_STATIONARY) == 2);
	CHECK(ubx_dynmodel_from_nav_mode(GPS_MODE_NORMAL) == 0);
}

int main(void)
{
	test_known_frames();
	test_pvt();
	test_resync_and_errors();
	test_long_frame_truncated();
	test_dop();
	test_valset();
	test_dynmodel();
	printf("%d checks, %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
