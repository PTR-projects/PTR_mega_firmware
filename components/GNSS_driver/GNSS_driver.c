/**
 * GNSS driver - common part.
 *
 * Owns the UART, auto-detects the receiver (u-blox UBX or NMEA/MTK), runs the parser task
 * and hands finished solutions (gps_t) to the consumer through a message buffer.
 * Protocol specifics live in GNSS_nmea.c and GNSS_ubx.c.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/message_buffer.h"

#include "esp_types.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "esp_log.h"

#include "driver/uart.h"

#include "BOARD_cfg.h"
#include "GNSS_driver.h"
#include "GNSS_priv.h"
#include "ubx_parser.h"

_Static_assert(sizeof(gps_t) <= 255, "gps_t must fit the uint8_t returned by GNSS_message_size()");

#if !(defined(GNSS_UART) && defined(GNSS_RX_PIN) && defined(GNSS_TX_PIN))
/* Board without GNSS receiver */
esp_err_t GPS_init() {return ESP_OK;}
esp_err_t GPS_init_type(gnss_type_t type) {(void)type; return ESP_OK;}
gnss_type_t GPS_getType(void) {return GNSS_TYPE_AUTO;}
uint8_t GNSS_message_size(void) {return sizeof(gps_t);}
esp_err_t GPS_checkStatus() {return ESP_OK;}
uint32_t GPS_getData(gps_t * data, uint16_t ms) {(void)data; (void)ms; return 0;}
void GPS_test(void) {}
esp_err_t GPS_baud_rate_set(uint32_t baud) {(void)baud; return ESP_OK;}
esp_err_t GPS_baud_rate_set_extra(uint32_t baud) {(void)baud; return ESP_OK;}
void GPS_fix_interval_set(uint16_t time) {(void)time;}
void GPS_nav_mode_set(gps_nav_mode_t mode) {(void)mode;}
void GPS_nmea_output_set(uint8_t GLL, uint8_t RMC, uint8_t VTG, uint8_t GGA, uint8_t GSA, uint8_t GSV)
{(void)GLL; (void)RMC; (void)VTG; (void)GGA; (void)GSA; (void)GSV;}

#else

static const char *TAG = "GNSS";

#define GNSS_UART_RX_BUF   CONFIG_NMEA_PARSER_RING_BUFFER_SIZE
#define GNSS_UART_TX_BUF   256
#define GNSS_STATS_PERIOD_US (5 * 1000000LL)

static MessageBufferHandle_t xMessageBuffer_GNSS2Storage;
static uint8_t xMessageBuffer_GNSS2Storage_buffer[1 + (4 + sizeof(gps_t)) * 10];
static StaticMessageBuffer_t  xMessageBuffer_GNSS2Storage_struct;

static volatile int64_t last_msg_timestamp = 0;
static const gnss_backend_t *s_backend = NULL;
static bool s_uart_installed = false;
static bool s_running = false;
static bool s_boot_wait_done = false;
static uint32_t s_baud = 0;

//---------------------------------------------
//	UART helpers (used by the backends)
//---------------------------------------------
esp_err_t gnss_write(const void *data, size_t len)
{
	int n = uart_write_bytes(GNSS_UART, data, len);
	return (n == (int)len) ? ESP_OK : ESP_FAIL;
}

int gnss_read(uint8_t *buf, size_t len, uint32_t timeout_ms)
{
	return uart_read_bytes(GNSS_UART, buf, len, pdMS_TO_TICKS(timeout_ms));
}

esp_err_t gnss_uart_set_baud(uint32_t baud)
{
	esp_err_t err = uart_set_baudrate(GNSS_UART, baud);
	if (err == ESP_OK)
		s_baud = baud;
	return err;
}

void gnss_uart_flush_rx(void)
{
	uart_flush_input(GNSS_UART);
}

//---------------------------------------------
//	Output to the rest of the system
//---------------------------------------------
void gnss_publish(gps_t *gps)
{
	static int64_t stat_t0 = 0;
	static uint32_t stat_cnt = 0;
	static bool stat_first = true;

	int64_t now = esp_timer_get_time();
	gps->rx_time_us = now;
	last_msg_timestamp = now;

	if (xMessageBufferSpacesAvailable(xMessageBuffer_GNSS2Storage) < (4 + sizeof(gps_t))) {
		ESP_LOGV(TAG, "Oldest packet removed");
		gps_t tmp_gps;
		xMessageBufferReceive(xMessageBuffer_GNSS2Storage, (void*)&tmp_gps, sizeof(gps_t), 0);
	}
	xMessageBufferSend(xMessageBuffer_GNSS2Storage, (void *)gps, sizeof(gps_t), 0);

	/* Measured solution rate - handy to verify the 20 Hz configuration on real hardware */
	if (stat_t0 == 0)
		stat_t0 = now;
	stat_cnt++;
	if ((now - stat_t0) >= GNSS_STATS_PERIOD_US) {
		float hz = stat_cnt * 1e6f / (float)(now - stat_t0);
		if (stat_first) {
			ESP_LOGI(TAG, "Solution rate %.1f Hz, fix %d, sats %u", hz, (int)gps->fix, gps->sats_in_use);
			stat_first = false;
		} else {
			ESP_LOGD(TAG, "Solution rate %.1f Hz, fix %d, sats %u", hz, (int)gps->fix, gps->sats_in_use);
		}
		stat_t0 = now;
		stat_cnt = 0;
	}
}

uint32_t GPS_getData(gps_t * data, uint16_t ms){
	return xMessageBufferReceive( xMessageBuffer_GNSS2Storage,
								 ( void * ) data,
								 sizeof( gps_t ),
								 pdMS_TO_TICKS( ms ));
}

esp_err_t GPS_checkStatus(){
	if((esp_timer_get_time() - last_msg_timestamp) > 5000000LL)
		return ESP_FAIL;

	return ESP_OK;
}

uint8_t GNSS_message_size(void)
{
	return sizeof(gps_t);
}

gnss_type_t GPS_getType(void)
{
	return s_backend ? s_backend->type : GNSS_TYPE_AUTO;
}

//---------------------------------------------
//	Receiver detection
//---------------------------------------------
typedef struct {
	bool ubx;          /*!< valid UBX frame received */
	bool pmtk;         /*!< "$PMTK" seen */
	bool nmea;         /*!< any "$G?" NMEA sentence seen */
	char win[6];       /*!< last received characters */
} probe_t;

static void probe_frame_cb(uint8_t cls, uint8_t id, const uint8_t *p, uint16_t len, void *ctx)
{
	(void)cls; (void)id; (void)p; (void)len;
	((probe_t *)ctx)->ubx = true;
}

static void probe_scan_text(probe_t *pr, const uint8_t *d, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		memmove(pr->win, pr->win + 1, 5);
		pr->win[5] = (char)d[i];
		if (memcmp(pr->win + 1, "$PMTK", 5) == 0)
			pr->pmtk = true;
		else if (pr->win[3] == '$' && pr->win[4] == 'G' && pr->win[5] >= 'A' && pr->win[5] <= 'Z')
			pr->nmea = true;
	}
}

/** Ask the receiver for its version in both dialects and see what comes back. */
static bool probe_baud(uint32_t baud, probe_t *pr)
{
	uint8_t frame[UBX_FRAME_OVERHEAD];
	static const char pmtk_query[] = "$PMTK605*31\r\n";
	uint8_t buf[64];
	ubx_parser_t parser;

	memset(pr, 0, sizeof(*pr));
	ubx_parser_init(&parser, probe_frame_cb, pr);

	gnss_uart_set_baud(baud);
	gnss_uart_flush_rx();

	/* Text query first: binary UBX bytes ahead of it could upset a simple line parser */
	gnss_write(pmtk_query, sizeof(pmtk_query) - 1);
	size_t n = ubx_build_frame(UBX_CLASS_MON, UBX_ID_MON_VER, NULL, 0, frame, sizeof(frame));
	gnss_write(frame, n);

	int64_t end = esp_timer_get_time() + 400 * 1000;
	while (esp_timer_get_time() < end) {
		int got = gnss_read(buf, sizeof(buf), 20);
		if (got > 0) {
			ubx_parser_feed(&parser, buf, (size_t)got);
			probe_scan_text(pr, buf, (size_t)got);
		}
		if (pr->ubx || pr->pmtk)
			break;
	}
	return pr->ubx || pr->pmtk || pr->nmea;
}

/** Find the receiver type and the baud rate it is currently using. */
static bool detect_receiver(gnss_type_t *type, uint32_t *baud)
{
	const uint32_t bauds[] = { GNSS_TARGET_BAUD, 9600 };   /* configured rate (warm restart), factory default */
	probe_t pr;

	for (int round = 0; round < 2; round++) {
		for (size_t i = 0; i < sizeof(bauds) / sizeof(bauds[0]); i++) {
			if (!probe_baud(bauds[i], &pr))
				continue;

			*baud = bauds[i];
			if (pr.ubx) {
				*type = GNSS_TYPE_UBLOX_UBX;
			} else {
				*type = GNSS_TYPE_NMEA_MTK;
				if (!pr.pmtk)
					ESP_LOGW(TAG, "NMEA receiver without PMTK reply - configuration commands may be ignored");
			}
			return true;
		}
	}
	return false;
}

//---------------------------------------------
//	Parser task
//---------------------------------------------
static void gnss_task_entry(void *arg)
{
	(void)arg;
	uint8_t buf[128];

	while (1) {
		int n = gnss_read(buf, sizeof(buf), 20);
		if (n > 0)
			s_backend->feed(buf, (size_t)n);
	}
}

//---------------------------------------------
//	Init
//---------------------------------------------
static esp_err_t uart_install(void)
{
	if (s_uart_installed)
		return ESP_OK;

	uart_config_t uart_config = {
		.baud_rate = GNSS_TARGET_BAUD,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
	};

	if (uart_driver_install(GNSS_UART, GNSS_UART_RX_BUF, GNSS_UART_TX_BUF, 0, NULL, 0) != ESP_OK) {
		ESP_LOGE(TAG, "install uart driver failed");
		return ESP_FAIL;
	}
	if (uart_param_config(GNSS_UART, &uart_config) != ESP_OK) {
		ESP_LOGE(TAG, "config uart parameter failed");
		uart_driver_delete(GNSS_UART);
		return ESP_FAIL;
	}
	/* GNSS_RX_PIN is the receiver's RX = ESP TX, GNSS_TX_PIN is the receiver's TX = ESP RX */
	if (uart_set_pin(GNSS_UART, GNSS_RX_PIN, GNSS_TX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK) {
		ESP_LOGE(TAG, "config uart gpio failed");
		uart_driver_delete(GNSS_UART);
		return ESP_FAIL;
	}
	s_baud = GNSS_TARGET_BAUD;
	s_uart_installed = true;
	return ESP_OK;
}

esp_err_t GPS_init(void)
{
	return GPS_init_type(GNSS_TYPE_DEFAULT);
}

esp_err_t GPS_init_type(gnss_type_t requested)
{
	if (s_running)
		return ESP_OK;

	//--------- Init ESP queue (once) -------------
	if (xMessageBuffer_GNSS2Storage == NULL) {
		xMessageBuffer_GNSS2Storage = xMessageBufferCreateStatic(
										sizeof(xMessageBuffer_GNSS2Storage_buffer),
										xMessageBuffer_GNSS2Storage_buffer,
										&xMessageBuffer_GNSS2Storage_struct);
		ESP_LOGI(TAG, "New queue created - size %u B, free spaces %u", (unsigned)sizeof(xMessageBuffer_GNSS2Storage_buffer),
				(unsigned)(xMessageBufferSpacesAvailable(xMessageBuffer_GNSS2Storage)/(4+sizeof(gps_t))));
	}

	//--------- Let the receiver boot, open UART -------------
	if (!s_boot_wait_done) {
		vTaskDelay(pdMS_TO_TICKS( 2000 ));
		s_boot_wait_done = true;
	}
	if (uart_install() != ESP_OK)
		return ESP_FAIL;

	//--------- Detect receiver -------------
	gnss_type_t type = GNSS_TYPE_NMEA_MTK;
	uint32_t baud = 9600;
	bool found = detect_receiver(&type, &baud);

	if (!found) {
		ESP_LOGW(TAG, "No GNSS receiver answered - assuming NMEA at %lu baud", (unsigned long)baud);
		gnss_uart_set_baud(baud);
	} else {
		ESP_LOGI(TAG, "Detected %s receiver at %lu baud",
				(type == GNSS_TYPE_UBLOX_UBX) ? "u-blox (UBX)" : "NMEA/MTK", (unsigned long)baud);
	}
	if (requested != GNSS_TYPE_AUTO) {
		if (found && requested != type)
			ESP_LOGW(TAG, "Forced receiver type differs from the detected one");
		type = requested;
	}
	s_backend = (type == GNSS_TYPE_UBLOX_UBX) ? &gnss_backend_ubx : &gnss_backend_nmea;

	//--------- Init GNSS receiver ----------------
	s_backend->reset();
	if (s_backend->configure(baud) != ESP_OK) {
		ESP_LOGE(TAG, "%s receiver configuration failed", s_backend->name);
		s_backend = NULL;
		return ESP_FAIL;
	}
	gnss_uart_flush_rx();

	//--------- Start parser task -------------
	BaseType_t err = xTaskCreatePinnedToCore(
						 gnss_task_entry,
						 "gnss_parser",
						 CONFIG_NMEA_PARSER_TASK_STACK_SIZE,
						 NULL,
						 CONFIG_NMEA_PARSER_TASK_PRIORITY,
						 NULL,
						 0);
	if (err != pdTRUE) {
		ESP_LOGE(TAG, "create GNSS parser task failed");
		s_backend = NULL;
		return ESP_FAIL;
	}

	s_running = true;
	ESP_LOGI(TAG, "Init ready (%s)", s_backend->name);
	return ESP_OK;
}

//---------------------------------------------
//	Runtime settings (dispatch to active backend)
//---------------------------------------------
static const gnss_backend_t *active_backend(void)
{
	return s_backend ? s_backend : &gnss_backend_nmea;
}

void GPS_test(void){
	ESP_LOGI(TAG, "test");
	active_backend()->poll_version();
}

esp_err_t GPS_baud_rate_set(uint32_t baud){
	return active_backend()->set_baud(baud, false);
}

esp_err_t GPS_baud_rate_set_extra(uint32_t baud){
	return active_backend()->set_baud(baud, true);
}

void GPS_fix_interval_set(uint16_t time){
	active_backend()->set_rate_ms(time);
}

void GPS_nav_mode_set(gps_nav_mode_t mode){
	active_backend()->set_nav_mode(mode);
}

#endif
