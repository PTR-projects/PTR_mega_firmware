/**
 * NMEA 0183 backend (MediaTek / PMTK receivers).
 * Parser derived from the ESP-IDF nmea_parser example, driven by a raw byte stream.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "driver/uart.h"

#include "BOARD_cfg.h"
#include "GNSS_driver.h"
#include "GNSS_priv.h"

#if defined(GNSS_UART) && defined(GNSS_RX_PIN) && defined(GNSS_TX_PIN)

#define NMEA_MAX_STATEMENT_ITEM_LENGTH 16

typedef enum {
	STATEMENT_UNKNOWN = 0, /*!< Unknown statement */
	STATEMENT_GGA,         /*!< GGA */
	STATEMENT_GSA,         /*!< GSA */
	STATEMENT_RMC,         /*!< RMC */
	STATEMENT_GSV,         /*!< GSV */
	STATEMENT_GLL,         /*!< GLL */
	STATEMENT_VTG          /*!< VTG */
} nmea_statement_t;

typedef struct {
	bool in_statement;                             /*!< '$' seen, collecting a statement */
	uint8_t item_pos;                              /*!< Current position in item */
	uint8_t item_num;                              /*!< Current item number */
	uint8_t asterisk;                              /*!< Asterisk detected flag */
	uint8_t crc;                                   /*!< Calculated CRC value */
	uint8_t parsed_statement;                      /*!< OR'd of statements that have been parsed */
	uint8_t cur_statement;                         /*!< Current statement ID */
	uint32_t all_statements;                       /*!< All statements mask */
	char item_str[NMEA_MAX_STATEMENT_ITEM_LENGTH]; /*!< Current item */
	gps_t parent;                                  /*!< Solution being assembled */
} nmea_state_t;

static const char *TAG = "GNSS-NMEA";
static nmea_state_t s_nmea;
static char s_tx[64];

static double parse_lat_long(const nmea_state_t *st)
{
	double ll = strtod(st->item_str, NULL);
	int deg = ((int)ll) / 100;
	double min = ll - (deg * 100);
	return deg + min / 60.0;
}

static inline uint8_t convert_two_digit2number(const char *digit_char)
{
	return 10 * (digit_char[0] - '0') + (digit_char[1] - '0');
}

static void parse_utc_time(nmea_state_t *st)
{
	if (strlen(st->item_str) < 6)
		return;
	st->parent.tim.hour = convert_two_digit2number(st->item_str + 0);
	st->parent.tim.minute = convert_two_digit2number(st->item_str + 2);
	st->parent.tim.second = convert_two_digit2number(st->item_str + 4);
	if (st->item_str[6] == '.') {
		uint16_t tmp = 0;
		uint8_t digits = 0;
		for (uint8_t i = 7; st->item_str[i] && digits < 3; i++, digits++)
			tmp = 10 * tmp + (st->item_str[i] - '0');
		for (; digits < 3; digits++)   // normalise to milliseconds
			tmp *= 10;
		st->parent.tim.thousand = tmp;
	}
}

#define KNOTS_TO_MPS 0.514444f

#if CONFIG_NMEA_STATEMENT_GGA
static void parse_gga(nmea_state_t *st)
{
	switch (st->item_num) {
	case 1: /* Process UTC time */
		parse_utc_time(st);
		break;
	case 2: /* Latitude */
		st->parent.latitude = parse_lat_long(st);
		break;
	case 3: /* Latitude north(1)/south(-1) information */
		if (st->item_str[0] == 'S' || st->item_str[0] == 's')
			st->parent.latitude *= -1;
		break;
	case 4: /* Longitude */
		st->parent.longitude = parse_lat_long(st);
		break;
	case 5: /* Longitude east(1)/west(-1) information */
		if (st->item_str[0] == 'W' || st->item_str[0] == 'w')
			st->parent.longitude *= -1;
		break;
	case 6: /* Fix status */
		st->parent.fix = (gps_fix_t)strtol(st->item_str, NULL, 10);
		st->parent.valid = (st->parent.fix != GPS_FIX_INVALID);
		break;
	case 7: /* Satellites in use */
		st->parent.sats_in_use = (uint8_t)strtol(st->item_str, NULL, 10);
		break;
	case 8: /* HDOP */
		st->parent.dop_h = strtof(st->item_str, NULL);
		break;
	case 9: /* Altitude above mean sea level */
		st->parent.altitude = strtof(st->item_str, NULL);
		st->parent.altitude_ellipsoid = st->parent.altitude;
		break;
	case 11: /* Geoid separation: ellipsoid height = MSL altitude + separation */
		st->parent.altitude_ellipsoid = st->parent.altitude + strtof(st->item_str, NULL);
		break;
	default:
		break;
	}
}
#endif

#if CONFIG_NMEA_STATEMENT_GSA
static void parse_gsa(nmea_state_t *st)
{
	switch (st->item_num) {
	case 2: /* Process fix mode */
		st->parent.fix_mode = (gps_fix_mode_t)strtol(st->item_str, NULL, 10);
		break;
	case 15: /* Process PDOP */
		st->parent.dop_p = strtof(st->item_str, NULL);
		break;
	case 16: /* Process HDOP */
		st->parent.dop_h = strtof(st->item_str, NULL);
		break;
	case 17: /* Process VDOP */
		st->parent.dop_v = strtof(st->item_str, NULL);
		break;
	default:
		break;
	}
}
#endif

#if CONFIG_NMEA_STATEMENT_RMC
static void parse_rmc(nmea_state_t *st)
{
	switch (st->item_num) {
	case 1:/* Process UTC time */
		parse_utc_time(st);
		break;
	case 2: /* Process valid status */
		st->parent.valid = (st->item_str[0] == 'A');
		break;
	case 3:/* Latitude */
		st->parent.latitude = parse_lat_long(st);
		break;
	case 4:
		if (st->item_str[0] == 'S' || st->item_str[0] == 's')
			st->parent.latitude *= -1;
		break;
	case 5:/* Longitude */
		st->parent.longitude = parse_lat_long(st);
		break;
	case 6:
		if (st->item_str[0] == 'W' || st->item_str[0] == 'w')
			st->parent.longitude *= -1;
		break;
	case 7: /* Process ground speed, knots -> m/s */
		st->parent.speed = strtof(st->item_str, NULL) * KNOTS_TO_MPS;
		break;
	case 8: /* Process true course over ground */
		st->parent.cog = strtof(st->item_str, NULL);
		break;
	case 9: /* Process date */
		if (strlen(st->item_str) >= 6) {
			st->parent.date.day = convert_two_digit2number(st->item_str + 0);
			st->parent.date.month = convert_two_digit2number(st->item_str + 2);
			st->parent.date.year = YEAR_BASE + convert_two_digit2number(st->item_str + 4);
		}
		break;
	case 10: /* Process magnetic variation */
		st->parent.variation = strtof(st->item_str, NULL);
		break;
	default:
		break;
	}
}
#endif

#if CONFIG_NMEA_STATEMENT_GLL
static void parse_gll(nmea_state_t *st)
{
	switch (st->item_num) {
	case 1:/* Latitude */
		st->parent.latitude = parse_lat_long(st);
		break;
	case 2:
		if (st->item_str[0] == 'S' || st->item_str[0] == 's')
			st->parent.latitude *= -1;
		break;
	case 3:/* Longitude */
		st->parent.longitude = parse_lat_long(st);
		break;
	case 4:
		if (st->item_str[0] == 'W' || st->item_str[0] == 'w')
			st->parent.longitude *= -1;
		break;
	case 5:/* Process UTC time */
		parse_utc_time(st);
		break;
	case 6: /* Process valid status */
		st->parent.valid = (st->item_str[0] == 'A');
		break;
	default:
		break;
	}
}
#endif

#if CONFIG_NMEA_STATEMENT_VTG
static void parse_vtg(nmea_state_t *st)
{
	switch (st->item_num) {
	case 1: /* Process true course over ground */
		st->parent.cog = strtof(st->item_str, NULL);
		break;
	case 3:/* Process magnetic course */
		st->parent.variation = strtof(st->item_str, NULL);
		break;
	case 5:/* Process ground speed, knots -> m/s */
		st->parent.speed = strtof(st->item_str, NULL) * KNOTS_TO_MPS;
		break;
	case 7:/* Process ground speed, km/h -> m/s */
		st->parent.speed = strtof(st->item_str, NULL) / 3.6f;
		break;
	default:
		break;
	}
}
#endif

static void parse_item(nmea_state_t *st)
{
	/* start of a statement: "$GPGGA" */
	if (st->item_num == 0 && st->item_str[0] == '$') {
		st->cur_statement = STATEMENT_UNKNOWN;
		if (0) {
		}
#if CONFIG_NMEA_STATEMENT_GGA
		else if (strstr(st->item_str, "GGA")) st->cur_statement = STATEMENT_GGA;
#endif
#if CONFIG_NMEA_STATEMENT_GSA
		else if (strstr(st->item_str, "GSA")) st->cur_statement = STATEMENT_GSA;
#endif
#if CONFIG_NMEA_STATEMENT_RMC
		else if (strstr(st->item_str, "RMC")) st->cur_statement = STATEMENT_RMC;
#endif
#if CONFIG_NMEA_STATEMENT_GLL
		else if (strstr(st->item_str, "GLL")) st->cur_statement = STATEMENT_GLL;
#endif
#if CONFIG_NMEA_STATEMENT_VTG
		else if (strstr(st->item_str, "VTG")) st->cur_statement = STATEMENT_VTG;
#endif
		return;
	}

	switch (st->cur_statement) {
#if CONFIG_NMEA_STATEMENT_GGA
	case STATEMENT_GGA: parse_gga(st); break;
#endif
#if CONFIG_NMEA_STATEMENT_GSA
	case STATEMENT_GSA: parse_gsa(st); break;
#endif
#if CONFIG_NMEA_STATEMENT_RMC
	case STATEMENT_RMC: parse_rmc(st); break;
#endif
#if CONFIG_NMEA_STATEMENT_GLL
	case STATEMENT_GLL: parse_gll(st); break;
#endif
#if CONFIG_NMEA_STATEMENT_VTG
	case STATEMENT_VTG: parse_vtg(st); break;
#endif
	default: break;
	}
}

static inline void item_add(nmea_state_t *st, char c)
{
	if (st->item_pos < NMEA_MAX_STATEMENT_ITEM_LENGTH - 1) {   // drop overlong items instead of overflowing
		st->item_str[st->item_pos++] = c;
		st->item_str[st->item_pos] = '\0';
	}
}

static void nmea_end_of_statement(nmea_state_t *st)
{
	uint8_t crc = (uint8_t)strtol(st->item_str, NULL, 16);
	if (st->asterisk && st->crc == crc) {
		switch (st->cur_statement) {
#if CONFIG_NMEA_STATEMENT_GGA
		case STATEMENT_GGA: st->parsed_statement |= 1 << STATEMENT_GGA; break;
#endif
#if CONFIG_NMEA_STATEMENT_GSA
		case STATEMENT_GSA: st->parsed_statement |= 1 << STATEMENT_GSA; break;
#endif
#if CONFIG_NMEA_STATEMENT_RMC
		case STATEMENT_RMC: st->parsed_statement |= 1 << STATEMENT_RMC; break;
#endif
#if CONFIG_NMEA_STATEMENT_GLL
		case STATEMENT_GLL: st->parsed_statement |= 1 << STATEMENT_GLL; break;
#endif
#if CONFIG_NMEA_STATEMENT_VTG
		case STATEMENT_VTG: st->parsed_statement |= 1 << STATEMENT_VTG; break;
#endif
		default: break;
		}

		/* All requested statements parsed -> solution complete */
		if ((st->parsed_statement & st->all_statements) == st->all_statements) {
			st->parsed_statement = 0;
			st->parent.source = GNSS_TYPE_NMEA_MTK;
			gnss_publish(&st->parent);
		}
	} else {
		ESP_LOGD(TAG, "CRC Error for statement type %d", (int)st->cur_statement);
	}
}

static void nmea_feed(const uint8_t *d, size_t len)
{
	nmea_state_t *st = &s_nmea;

	for (size_t i = 0; i < len; i++) {
		char c = (char)d[i];

		if (c == '$') {                        /* Start of a statement: reset runtime information */
			st->in_statement = true;
			st->asterisk = 0;
			st->item_num = 0;
			st->item_pos = 0;
			st->cur_statement = STATEMENT_UNKNOWN;
			st->crc = 0;
			item_add(st, c);
		} else if (!st->in_statement) {
			continue;                          /* ignore anything outside of a statement */
		} else if (c == ',') {                 /* Item separator */
			parse_item(st);
			st->crc ^= (uint8_t)c;
			st->item_pos = 0;
			st->item_str[0] = '\0';
			st->item_num++;
		} else if (c == '*') {                 /* End of CRC computation */
			parse_item(st);
			st->asterisk = 1;
			st->item_pos = 0;
			st->item_str[0] = '\0';
			st->item_num++;
		} else if (c == '\r' || c == '\n') {   /* End of statement */
			if (st->asterisk)
				nmea_end_of_statement(st);
			st->in_statement = false;
		} else {
			if (!st->asterisk)
				st->crc ^= (uint8_t)c;
			item_add(st, c);
		}
	}
}

//---------------------------------------------
//	PMTK commands
//---------------------------------------------
static uint8_t crc_calc(const char *message)
{
	uint8_t crc = 0;
	for (size_t i = 0; message[i]; i++)
		crc ^= (uint8_t)message[i];
	return crc;
}

static esp_err_t send_cmd(const char *message)
{
	snprintf(s_tx, sizeof(s_tx), "$%s*%02X\r\n", message, crc_calc(message));
	return gnss_write(s_tx, strlen(s_tx));
}

static bool baud_valid(uint32_t baud)
{
	return baud == 4800 || baud == 9600 || baud == 14400 || baud == 19200 ||
	       baud == 38400 || baud == 57600 || baud == 115200;
}

static esp_err_t nmea_set_baud(uint32_t baud, bool set_uart)
{
	char message[24];
	if (!baud_valid(baud)) {
		ESP_LOGE(TAG, "WRONG BAUDRATE");
		return set_uart ? ESP_ERR_INVALID_ARG : ESP_FAIL;
	}
	snprintf(message, sizeof(message), "PMTK251,%lu", (unsigned long)baud);

	esp_err_t status = send_cmd(message);
	if (status == ESP_OK && set_uart) {
		status = uart_wait_tx_done(GNSS_UART, portMAX_DELAY);
		if (status == ESP_OK)
			status = gnss_uart_set_baud(baud);
	}
	if (status == ESP_OK)
		ESP_LOGI(TAG, "GPS%s baudrate set to %lu", set_uart ? " and UART" : "", (unsigned long)baud);
	return status;
}

static void nmea_set_rate_ms(uint16_t time)
{
	char message[24];
	if (time < 100 || time > 10000) {
		ESP_LOGE(TAG, "Fix interval out of range (100-10000 ms)");
		return;
	}
	snprintf(message, sizeof(message), "PMTK220,%u", (unsigned)time);
	send_cmd(message);
}

static void nmea_set_nav_mode(gps_nav_mode_t mode)
{
	char message[24];
	snprintf(message, sizeof(message), "PMTK886,%i", (int)mode);
	send_cmd(message);
}

static void nmea_poll_version(void)
{
	send_cmd("PMTK605");
}

void GPS_nmea_output_set(uint8_t GLL, uint8_t RMC, uint8_t VTG, uint8_t GGA, uint8_t GSA, uint8_t GSV)
{
	char message[64];
	uint8_t *rates[] = { &GLL, &RMC, &VTG, &GGA, &GSA, &GSV };
	const char *names[] = { "GLL", "RMC", "VTG", "GGA", "GSA", "GSV" };

	if (GPS_getType() == GNSS_TYPE_UBLOX_UBX) {
		ESP_LOGW(TAG, "NMEA output selection is not used with a u-blox receiver");
		return;
	}
	for (int i = 0; i < 6; i++) {
		if (*rates[i] > 5) {
			*rates[i] = 5;
			ESP_LOGE(TAG, "NMEA OUTPUT %s freq parameter too high set 5 instead", names[i]);
		}
	}
	snprintf(message, sizeof(message), "PMTK314,%i,%i,%i,%i,%i,%i,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0",
	         GLL, RMC, VTG, GGA, GSA, GSV);
	send_cmd(message);
	ESP_LOGI(TAG, "NMEA OUTPUT SET TO: GLL %d RMC %d VTG %d GGA %d GSA %d GSV %d", GLL, RMC, VTG, GGA, GSA, GSV);
}

//---------------------------------------------
//	Backend
//---------------------------------------------
static void nmea_reset(void)
{
	memset(&s_nmea, 0, sizeof(s_nmea));
#if CONFIG_NMEA_STATEMENT_GSA
	s_nmea.all_statements |= (1 << STATEMENT_GSA);
#endif
#if CONFIG_NMEA_STATEMENT_GGA
	s_nmea.all_statements |= (1 << STATEMENT_GGA);
#endif
#if CONFIG_NMEA_STATEMENT_RMC
	s_nmea.all_statements |= (1 << STATEMENT_RMC);
#endif
#if CONFIG_NMEA_STATEMENT_GLL
	s_nmea.all_statements |= (1 << STATEMENT_GLL);
#endif
#if CONFIG_NMEA_STATEMENT_VTG
	s_nmea.all_statements |= (1 << STATEMENT_VTG);
#endif
}

/* Output rate (every N fixes) requested from the receiver for each statement that the parser handles */
#if CONFIG_NMEA_STATEMENT_GGA
#define NMEA_OUT_GGA 1
#else
#define NMEA_OUT_GGA 0
#endif
#if CONFIG_NMEA_STATEMENT_GSA
#define NMEA_OUT_GSA 1
#else
#define NMEA_OUT_GSA 0
#endif
#if CONFIG_NMEA_STATEMENT_RMC
#define NMEA_OUT_RMC 1
#else
#define NMEA_OUT_RMC 0
#endif
#if CONFIG_NMEA_STATEMENT_GLL
#define NMEA_OUT_GLL 1
#else
#define NMEA_OUT_GLL 0
#endif
#if CONFIG_NMEA_STATEMENT_VTG
#define NMEA_OUT_VTG 1
#else
#define NMEA_OUT_VTG 0
#endif

static esp_err_t nmea_configure(uint32_t baud)
{
	/* Statements the parser waits for must match what the receiver is told to output */
	if (baud != GNSS_TARGET_BAUD) {
		nmea_set_baud(GNSS_TARGET_BAUD, true);
		vTaskDelay(pdMS_TO_TICKS(100));
	}
	GPS_nmea_output_set(NMEA_OUT_GLL, NMEA_OUT_RMC, NMEA_OUT_VTG, NMEA_OUT_GGA, NMEA_OUT_GSA, 0);
	vTaskDelay(pdMS_TO_TICKS(100));
	nmea_set_nav_mode(GNSS_NAV_MODE_DEFAULT);                  vTaskDelay(pdMS_TO_TICKS(100));
	nmea_set_rate_ms(GNSS_NMEA_RATE_MS);                       vTaskDelay(pdMS_TO_TICKS(100));
	return ESP_OK;
}

const gnss_backend_t gnss_backend_nmea = {
	.type = GNSS_TYPE_NMEA_MTK,
	.name = "NMEA/MTK",
	.reset = nmea_reset,
	.configure = nmea_configure,
	.feed = nmea_feed,
	.set_baud = nmea_set_baud,
	.set_rate_ms = nmea_set_rate_ms,
	.set_nav_mode = nmea_set_nav_mode,
	.poll_version = nmea_poll_version,
};

#endif /* GNSS pins defined */
