#pragma once

/**
 * @file GNSS_priv.h
 * @brief Internal interface between the common GNSS driver and its protocol backends.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "GNSS_driver.h"

/* Receiver settings (fixed for this board) */
#define GNSS_TARGET_BAUD      115200            /*!< baud rate used after configuration */
#define GNSS_NAV_MODE_DEFAULT GPS_MODE_AVIATION /*!< navigation / platform model */

typedef struct {
	gnss_type_t type;
	const char *name;

	/** Prepare backend state; called once after the receiver was detected, before configure(). */
	void (*reset)(void);
	/**
	 * Configure the receiver (rate, navigation mode, output messages, baud rate).
	 * The UART is open at `baud`. The parser task is not running yet, so the backend may
	 * read the UART directly. On return the UART must be at the baud rate the receiver uses.
	 */
	esp_err_t (*configure)(uint32_t baud);
	/** Consume raw bytes received from the receiver (called from the parser task). */
	void (*feed)(const uint8_t *data, size_t len);

	esp_err_t (*set_baud)(uint32_t baud, bool set_uart);
	void (*set_rate_ms)(uint16_t ms);
	void (*set_nav_mode)(gps_nav_mode_t mode);
	void (*poll_version)(void);
} gnss_backend_t;

extern const gnss_backend_t gnss_backend_nmea;
extern const gnss_backend_t gnss_backend_ubx;

/** Publish a finished solution to the consumer queue and update the watchdog timestamp. */
void gnss_publish(gps_t *gps);

/** Write raw bytes to the receiver. Returns ESP_OK on success. */
esp_err_t gnss_write(const void *data, size_t len);

/** Read raw bytes from the receiver (blocking up to timeout_ms). Returns number of bytes. */
int gnss_read(uint8_t *buf, size_t len, uint32_t timeout_ms);

/** Change baud rate of the ESP side UART. */
esp_err_t gnss_uart_set_baud(uint32_t baud);

/** Discard everything in the UART RX ring buffer. */
void gnss_uart_flush_rx(void);
