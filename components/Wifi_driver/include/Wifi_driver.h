#ifndef WIFI_DRIVER_H
#define WIFI_DRIVER_H

#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"

/**
 * @brief Represents the operational status of the Wi-Fi module.
 */
typedef enum wifi_status_t{
    WIFI_INACTIVE = 0, /*!< Wi-Fi is disabled or not initialized. */
    WIFI_ACTIVE = 1,   /*!< Wi-Fi is active, and the Soft AP is running. */
}wifi_status_t;

/**
 * @brief Initializes the Wi-Fi driver and starts the Soft AP.
 *
 * This function performs the following steps:
 * 1. Initializes NVS (Non-Volatile Storage).
 * 2. Initializes the TCP/IP stack and the default event loop.
 * 3. Creates the default Wi-Fi AP network interface.
 * 4. Configures the AP with SSID and password from Kconfig or preferences.
 * 5. Starts Wi-Fi in AP mode.
 *
 * @return
 *  - ESP_OK on successful Wi-Fi AP start.
 *  - Other esp_err_t codes on failure during initialization of NVS, netif, or Wi-Fi.
 */
esp_err_t wifi_enable(void);

/**
 * @brief Stops the Soft AP and deinitializes the Wi-Fi driver.
 *
 * This function stops Wi-Fi, deinitializes the driver, and frees associated resources.
 *
 * @return
 *  - ESP_OK on success.
 *  - Other esp_err_t codes on failure.
 */
esp_err_t wifi_disable(void);

/**
 * @brief Gets the current status of the Wi-Fi module.
 *
 * @return The current status as a Wifi_status_t enum value.
 */
wifi_status_t get_wifi_status(void);

#endif // WIFI_DRIVER_H
