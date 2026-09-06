#include <string.h>

#include "Wifi_driver.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "Preferences.h"

static const char *TAG = "WIFI";

#define WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define CONFIG_ESP_WIFI_CHANEL   1
#define CONFIG_ESP_WIFI_MAX_CONNECTIONS   1

static wifi_status_t wifi_status = WIFI_INACTIVE;
static esp_netif_t *wifi_ap_netif = NULL;

esp_err_t wifi_enable(void){
    esp_err_t ret = nvs_flash_init();

    if(wifi_status == WIFI_ACTIVE)
    {
        ESP_LOGI(TAG, "WIFI was already enabled");
        return ESP_OK;
    }

	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
		ESP_LOGW(TAG, "NVS partition was corrupted or full, erasing...");
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init(); // Retry NVS init
	}
	if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS flash: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_netif_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize netif: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) { // ESP_ERR_INVALID_STATE means it's already created
        ESP_LOGE(TAG, "Failed to create default event loop: %s", esp_err_to_name(ret));
        return ret;
    }

    wifi_ap_netif = esp_netif_create_default_wifi_ap();
    if (wifi_ap_netif == NULL) {
        ESP_LOGE(TAG, "Failed to create default Wi-Fi AP netif");
        return ESP_FAIL;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if ((ret = esp_wifi_init(&cfg)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    wifi_config_t wifi_config = {
    	.ap = {
    		.ssid = CONFIG_ESP_WIFI_SSID,
    		.ssid_len = strlen(CONFIG_ESP_WIFI_SSID),
    		.channel = CONFIG_ESP_WIFI_CHANEL,
    		.password =  CONFIG_ESP_WIFI_PASSWORD,
    		.max_connection = CONFIG_ESP_WIFI_MAX_CONNECTIONS,
    		.authmode = WIFI_AUTH_WPA_WPA2_PSK
    	},
    };

    Preferences_data_t pref;
    if(Preferences_get(&pref) == ESP_OK){
    	strlcpy((char *)wifi_config.ap.password, pref.wifi_pass, sizeof(wifi_config.ap.password));
    	ESP_LOGV(TAG, "WiFi Pass from pref.: %s", (char *)wifi_config.ap.password);
    } else {
        ESP_LOGW(TAG, "Could not get preferences, using default password from Kconfig.");
    }

    if (strlen((char*)wifi_config.ap.password) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        ESP_LOGI(TAG, "WiFi Open");
    }

    if ((ret = esp_wifi_set_mode(WIFI_MODE_AP)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_mode failed: %s", esp_err_to_name(ret));
        return ret;
    }
    if ((ret = esp_wifi_set_config(WIFI_IF_AP, &wifi_config)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_config failed: %s", esp_err_to_name(ret));
        return ret;
    }
    if ((ret = esp_wifi_start()) != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_start failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Soft AP initialization finished. SSID: %s password: %s channel: %d", wifi_config.ap.ssid, wifi_config.ap.password, wifi_config.ap.channel);

    esp_netif_ip_info_t ip_info;
    if ((ret = esp_netif_get_ip_info(wifi_ap_netif, &ip_info)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_netif_get_ip_info failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG,"IP Address:  " IPSTR, IP2STR(&ip_info.ip));
    ESP_LOGI(TAG,"Subnet mask: " IPSTR, IP2STR(&ip_info.netmask));
    ESP_LOGI(TAG,"Gateway:     " IPSTR, IP2STR(&ip_info.gw));

    wifi_status = WIFI_ACTIVE;
    return ESP_OK;
}

esp_err_t wifi_disable(void)
{
    esp_err_t ret;

    if (wifi_status == WIFI_INACTIVE) {
        ESP_LOGW(TAG, "Wi-Fi is already inactive.");
        return ESP_OK;
    }

    ret = esp_wifi_stop();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop Wi-Fi: %s", esp_err_to_name(ret));
        // Continue with deinitialization anyway
    } else {
        ESP_LOGI(TAG, "Wi-Fi stopped.");
    }

    ret = esp_wifi_deinit();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to deinitialize Wi-Fi: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Wi-Fi deinitialized.");

    if (wifi_ap_netif) {
        esp_netif_destroy_default_wifi(wifi_ap_netif);
        wifi_ap_netif = NULL;
    }

    wifi_status = WIFI_INACTIVE;
    return ESP_OK;
}

wifi_status_t get_wifi_status(void)
{
    return wifi_status;
}
