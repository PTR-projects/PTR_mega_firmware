/**
 * u-blox UBX backend, target receiver: MAX-M10N (SPG 5.30). Other M10 modules use the same
 * keys; the MAX-M10S additionally supports GLONASS.
 *
 * The receiver is configured with UBX-CFG-VALSET (RAM layer, so a power cycle returns it to
 * its defaults - the auto-detection in GPS_init() copes with that) and delivers one
 * UBX-NAV-PVT per navigation epoch, optionally accompanied by UBX-NAV-DOP at 1 Hz.
 */
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "driver/uart.h"

#include "BOARD_cfg.h"
#include "GNSS_driver.h"
#include "GNSS_priv.h"
#include "ubx_parser.h"

#if defined(GNSS_UART) && defined(GNSS_RX_PIN) && defined(GNSS_TX_PIN)

/* Constellations. On the MAX-M10N 20 Hz is only possible with GPS+Galileo (single
 * constellation: 25 Hz, the default GPS+GAL+BDS set: 10 Hz), see the MAX-M10N data sheet.
 * The MAX-M10N has no GLONASS, so that key is only sent when GNSS_UBX_USE_GLO is defined. */
#ifndef GNSS_UBX_USE_GPS
#define GNSS_UBX_USE_GPS 1
#endif
#ifndef GNSS_UBX_USE_GAL
#define GNSS_UBX_USE_GAL 1
#endif
#ifndef GNSS_UBX_USE_BDS
#define GNSS_UBX_USE_BDS 0
#endif
#ifndef GNSS_UBX_USE_SBAS
#define GNSS_UBX_USE_SBAS 0
#endif
#ifndef GNSS_UBX_USE_QZSS
#define GNSS_UBX_USE_QZSS 0
#endif
/** Request UBX-NAV-DOP (HDOP/VDOP) at 1 Hz; 0 = HDOP is not provided */
#ifndef GNSS_UBX_ENABLE_DOP
#define GNSS_UBX_ENABLE_DOP 1
#endif

#define UBX_ACK_TIMEOUT_MS 300

static const char *TAG = "GNSS-UBX";

static ubx_parser_t s_parser;
static gps_t s_cur;                        /*!< solution being published, keeps DOP between epochs */
static volatile bool s_waiting;            /*!< configure() is waiting for an ACK */
static volatile uint8_t s_wait_cls, s_wait_id;
static volatile int s_ack_result;          /*!< 0 pending, 1 ACK, -1 NAK */
static volatile bool s_ver_seen;
static uint32_t s_pvt_count;

static void on_frame(uint8_t cls, uint8_t id, const uint8_t *p, uint16_t len, void *ctx)
{
	(void)ctx;

	if (cls == UBX_CLASS_NAV && id == UBX_ID_NAV_PVT) {
		ubx_nav_pvt_t pvt;
		if (ubx_decode_nav_pvt(p, len, &pvt)) {
			ubx_pvt_to_gps(&pvt, &s_cur);
			s_pvt_count++;
			gnss_publish(&s_cur);
		}
	} else if (cls == UBX_CLASS_NAV && id == UBX_ID_NAV_DOP) {
		ubx_nav_dop_t dop;
		if (ubx_decode_nav_dop(p, len, &dop)) {
			s_cur.dop_h = dop.h_dop;
			s_cur.dop_v = dop.v_dop;
			s_cur.dop_p = dop.p_dop;
		}
	} else if (cls == UBX_CLASS_ACK && len >= 2) {
		bool ack = (id == UBX_ID_ACK_ACK);
		if (!ack)
			ESP_LOGW(TAG, "Receiver rejected message 0x%02X 0x%02X (NAK)", p[0], p[1]);
		if (s_waiting && p[0] == s_wait_cls && p[1] == s_wait_id)
			s_ack_result = ack ? 1 : -1;
	} else if (cls == UBX_CLASS_MON && id == UBX_ID_MON_VER && len >= 40) {
		char sw[31], hw[11];
		memcpy(sw, p, 30);      sw[30] = '\0';
		memcpy(hw, p + 30, 10); hw[10] = '\0';
		ESP_LOGI(TAG, "u-blox receiver: SW '%s' HW '%s'", sw, hw);
		s_ver_seen = true;
	}
}

static void ubx_feed(const uint8_t *data, size_t len)
{
	ubx_parser_feed(&s_parser, data, len);
}

static void ubx_reset(void)
{
	ubx_parser_init(&s_parser, on_frame, NULL);
	memset(&s_cur, 0, sizeof(s_cur));
	s_cur.source = GNSS_TYPE_UBLOX_UBX;
	s_pvt_count = 0;
}

/** Read from the receiver for up to timeout_ms, feeding the parser, until *done becomes true. */
static void pump(uint32_t timeout_ms, volatile int *ack_done, volatile bool *flag_done)
{
	uint8_t buf[64];
	int64_t end = esp_timer_get_time() + (int64_t)timeout_ms * 1000;

	while (esp_timer_get_time() < end) {
		int n = gnss_read(buf, sizeof(buf), 20);
		if (n > 0)
			ubx_parser_feed(&s_parser, buf, (size_t)n);
		if ((ack_done && *ack_done != 0) || (flag_done && *flag_done))
			return;
	}
}

/** Send a frame and (optionally) wait for the receiver's ACK. */
static esp_err_t send_frame(const uint8_t *frame, size_t n, bool wait)
{
	if (n == 0)
		return ESP_ERR_INVALID_SIZE;
	if (wait) {
		s_wait_cls = frame[2];
		s_wait_id = frame[3];
		s_ack_result = 0;
		s_waiting = true;
	}
	esp_err_t err = gnss_write(frame, n);
	if (err == ESP_OK && wait) {
		pump(UBX_ACK_TIMEOUT_MS, &s_ack_result, NULL);
		err = (s_ack_result == 1) ? ESP_OK : (s_ack_result == -1 ? ESP_FAIL : ESP_ERR_TIMEOUT);
	}
	s_waiting = false;
	return err;
}

static esp_err_t send_valset(const ubx_cfg_item_t *items, size_t n, bool wait)
{
	uint8_t frame[64];
	size_t len = ubx_build_valset(UBX_LAYER_RAM, items, n, frame, sizeof(frame));
	return send_frame(frame, len, wait);
}

static bool poll_ver_wait(uint32_t timeout_ms)
{
	uint8_t frame[UBX_FRAME_OVERHEAD];
	size_t n = ubx_build_frame(UBX_CLASS_MON, UBX_ID_MON_VER, NULL, 0, frame, sizeof(frame));

	s_ver_seen = false;
	if (gnss_write(frame, n) != ESP_OK)
		return false;
	pump(timeout_ms, NULL, &s_ver_seen);
	return s_ver_seen;
}

static void ubx_poll_version(void)
{
	uint8_t frame[UBX_FRAME_OVERHEAD];
	size_t n = ubx_build_frame(UBX_CLASS_MON, UBX_ID_MON_VER, NULL, 0, frame, sizeof(frame));
	gnss_write(frame, n);   // answer is logged by the parser task
}

static esp_err_t ubx_set_baud(uint32_t baud, bool set_uart)
{
	if (baud < 4800 || baud > 921600)
		return ESP_ERR_INVALID_ARG;

	ubx_cfg_item_t item = { UBX_KEY_UART1_BAUDRATE, baud };
	esp_err_t err = send_valset(&item, 1, false);
	if (err == ESP_OK && set_uart) {
		err = uart_wait_tx_done(GNSS_UART, portMAX_DELAY);
		vTaskDelay(pdMS_TO_TICKS(50));       // receiver switches after the message was processed
		if (err == ESP_OK)
			err = gnss_uart_set_baud(baud);
		gnss_uart_flush_rx();
	}
	if (err == ESP_OK)
		ESP_LOGI(TAG, "GPS%s baudrate set to %lu", set_uart ? " and UART" : "", (unsigned long)baud);
	return err;
}

static void ubx_set_rate_ms(uint16_t ms)
{
	if (ms < 40 || ms > 10000) {
		ESP_LOGE(TAG, "Fix interval out of range (40-10000 ms)");
		return;
	}
	ubx_cfg_item_t items[] = { { UBX_KEY_RATE_MEAS, ms }, { UBX_KEY_RATE_NAV, 1 } };
	send_valset(items, 2, false);
}

static void ubx_set_nav_mode(gps_nav_mode_t mode)
{
	ubx_cfg_item_t item = { UBX_KEY_NAVSPG_DYNMODEL, ubx_dynmodel_from_nav_mode(mode) };
	send_valset(&item, 1, false);
}

static esp_err_t ubx_configure(uint32_t baud)
{
	esp_err_t err;
	uint32_t cur_baud = baud;

	/* 1. Make sure the receiver talks UBX */
	bool alive = false;
	for (int i = 0; i < 3 && !alive; i++)
		alive = poll_ver_wait(300);
	if (!alive) {
		ESP_LOGE(TAG, "No UBX reply from receiver at %lu baud", (unsigned long)baud);
		return ESP_FAIL;
	}

	/* 2. Faster UART first: 20 Hz of NAV-PVT does not fit in 9600 baud */
	if (cur_baud != GNSS_TARGET_BAUD) {
		ubx_set_baud(GNSS_TARGET_BAUD, true);
		if (poll_ver_wait(500) || poll_ver_wait(500)) {
			cur_baud = GNSS_TARGET_BAUD;
		} else {
			ESP_LOGE(TAG, "No reply after baud change, going back to %lu", (unsigned long)baud);
			gnss_uart_set_baud(baud);
			gnss_uart_flush_rx();
			if (!poll_ver_wait(500))
				return ESP_FAIL;
		}
	}

	/* 3. Protocols and messages: UBX only, NAV-PVT every epoch (essential, must be ACKed) */
	ubx_cfg_item_t proto[] = {
		{ UBX_KEY_UART1INPROT_UBX,      1 },
		{ UBX_KEY_UART1OUTPROT_UBX,     1 },
		{ UBX_KEY_UART1OUTPROT_NMEA,    0 },
		{ UBX_KEY_MSGOUT_NAV_PVT_UART1, 1 },
	};
	err = send_valset(proto, sizeof(proto) / sizeof(proto[0]), true);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Protocol/message configuration failed (%s)", esp_err_to_name(err));
		return ESP_FAIL;
	}

	/* 4. Constellations (optional - a NAK here only costs update rate) */
	ubx_cfg_item_t sig[] = {
		{ UBX_KEY_SIGNAL_GPS_ENA,  GNSS_UBX_USE_GPS  },
		{ UBX_KEY_SIGNAL_GAL_ENA,  GNSS_UBX_USE_GAL  },
#ifdef GNSS_UBX_USE_GLO
		{ UBX_KEY_SIGNAL_GLO_ENA,  GNSS_UBX_USE_GLO  },
#endif
		{ UBX_KEY_SIGNAL_BDS_ENA,  GNSS_UBX_USE_BDS  },
		{ UBX_KEY_SIGNAL_SBAS_ENA, GNSS_UBX_USE_SBAS },
		{ UBX_KEY_SIGNAL_QZSS_ENA, GNSS_UBX_USE_QZSS },
	};
	/* One message per key: a single unsupported key (e.g. a constellation the module lacks) makes the
	 * receiver NAK the whole VALSET, which would silently drop the other constellations too. */
	for (size_t i = 0; i < sizeof(sig) / sizeof(sig[0]); i++) {
		if (send_valset(&sig[i], 1, true) != ESP_OK)
			ESP_LOGW(TAG, "Constellation key 0x%08lX not accepted, update rate may be limited", (unsigned long)sig[i].key);
	}

	/* 5. Navigation rate and platform model */
	ubx_cfg_item_t rate[] = {
		{ UBX_KEY_RATE_MEAS, GNSS_UBX_RATE_MS },
		{ UBX_KEY_RATE_NAV,  1 },
		{ UBX_KEY_NAVSPG_DYNMODEL, ubx_dynmodel_from_nav_mode(GNSS_NAV_MODE_DEFAULT) },
	};
	err = send_valset(rate, sizeof(rate) / sizeof(rate[0]), true);
	if (err != ESP_OK)
		ESP_LOGW(TAG, "Rate/platform model not confirmed (%s)", esp_err_to_name(err));

#if GNSS_UBX_ENABLE_DOP
	/* 6. Optional: HDOP/VDOP once per second */
	ubx_cfg_item_t dop = { UBX_KEY_MSGOUT_NAV_DOP_UART1, (1000u / GNSS_UBX_RATE_MS) ? (1000u / GNSS_UBX_RATE_MS) : 1u };
	if (send_valset(&dop, 1, true) != ESP_OK)
		ESP_LOGW(TAG, "NAV-DOP not enabled, dop_h/dop_v stay 0");
#endif

	ESP_LOGI(TAG, "Configured: %u ms rate, %lu baud", (unsigned)GNSS_UBX_RATE_MS, (unsigned long)cur_baud);
	return ESP_OK;
}

const gnss_backend_t gnss_backend_ubx = {
	.type = GNSS_TYPE_UBLOX_UBX,
	.name = "u-blox UBX",
	.reset = ubx_reset,
	.configure = ubx_configure,
	.feed = ubx_feed,
	.set_baud = ubx_set_baud,
	.set_rate_ms = ubx_set_rate_ms,
	.set_nav_mode = ubx_set_nav_mode,
	.poll_version = ubx_poll_version,
};

#endif /* GNSS pins defined */
