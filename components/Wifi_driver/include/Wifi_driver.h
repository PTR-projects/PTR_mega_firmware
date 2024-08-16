#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"

typedef enum Wifi_status_t{
    WIFI_INACTIVE = 0,
    WIFI_ACTIVE = 1,
}Wifi_status_t;

esp_err_t Wifi_enable(void);
esp_err_t Wifi_disable(void);
Wifi_status_t Wifi_status(void);