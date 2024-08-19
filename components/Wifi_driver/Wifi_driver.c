#include <string.h>

#include "Wifi_driver.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "Preferences.h"

static const char *TAG = "WIFI";

//#define WIFI_SSID      CONFIG_ESP_WIFI_SSID
//#define WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD

#define CONFIG_ESP_WIFI_CHANEL   1
#define CONFIG_ESP_WIFI_MAX_CONNECTIONS   1

static Wifi_status_t wifi_status = WIFI_INACTIVE;
static esp_netif_t *esp_netif; 

/*!
 * @brief Initialize wifi, create soft access point.
 * @return `ESP_OK` if initialized
 * @return `ESP_FAIL` otherwise.
 */
esp_err_t Wifi_enable(void){
    esp_err_t ret = nvs_flash_init();

    if(wifi_status == WIFI_ACTIVE)
    {
        ESP_LOGI(TAG, "WIFI was already enabled");
        return ESP_OK;
    }

	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
		ESP_ERROR_CHECK(nvs_flash_erase()); //potencjalnie niebezpieczne
		ret = nvs_flash_init();
	}

	ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_netif_init());

    if (esp_event_loop_create_default() != ESP_OK) {
        ESP_LOGW(TAG, "Event loop already created. Skipping...");
    }

    esp_netif = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_LOGW(TAG, "Default Wi-Fi AP netif already created.");

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
    	memset((void *)wifi_config.ap.password, 0, sizeof(wifi_config.ap.password));
    	strncpy((char *)wifi_config.ap.password, pref.wifi_pass, sizeof(wifi_config.ap.password));
    	ESP_LOGV(TAG, "WiFi Pass from pref.: %s", (char *)wifi_config.ap.password);
    }
    else if (strlen(CONFIG_ESP_WIFI_PASSWORD) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        ESP_LOGI(TAG, "WiFi Open");
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Soft AP initialization finished. SSID: %s password: %s channel: %d", wifi_config.ap.ssid, wifi_config.ap.password, wifi_config.ap.channel);

    tcpip_adapter_ip_info_t ip_info;
    ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &ip_info));

    ESP_LOGI(TAG,"IP Address:  %s", ip4addr_ntoa(&ip_info.ip));
    ESP_LOGI(TAG,"Subnet mask: %s", ip4addr_ntoa(&ip_info.netmask));
    ESP_LOGI(TAG,"Gateway:     %s", ip4addr_ntoa(&ip_info.gw));

    wifi_status = WIFI_ACTIVE;
    return ESP_OK;
}

esp_err_t Wifi_disable(void)
{
    esp_err_t ret;

    ret = esp_wifi_stop();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Wi-Fi successfully stopped.");
        wifi_status = WIFI_INACTIVE;
    } else if (ret == ESP_ERR_WIFI_NOT_INIT) {
        ESP_LOGW(TAG, "Wi-Fi was not initialized when trying do stop it");
    } else {
        ESP_LOGE(TAG, "Failed to stop Wi-Fi: %s", esp_err_to_name(ret));
        return;
    }
    vTaskDelay(pdMS_TO_TICKS( 60000 ));
    ret = esp_wifi_deinit();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Wi-Fi successfully deinitialized.");
    } else {
        ESP_LOGE(TAG, "Failed to deinitialize Wi-Fi: %s", esp_err_to_name(ret));
    }
    esp_netif_destroy_default_wifi(esp_netif);
    return ret;
}

Wifi_status_t Wifi_status(void)
{
    return wifi_status;
}