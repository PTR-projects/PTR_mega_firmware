

/*  WiFi softAP Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "web_driver.h"

static const char *TAG = "Web_driver";

#define WIFI_SSID      "KPPTR"
#define WIFI_PASS      "123456789"
#define WIFI_CHANNEL   1
#define MAX_STA_CONN   1



esp_err_t Web_wifi_init 					(void);
httpd_handle_t Web_http_init 				(void);
void Web_http_stop							(httpd_handle_t server);
static esp_err_t index_html_get_handler		(httpd_req_t *req);



esp_err_t Web_init(void){
	esp_err_t ret = ESP_FAIL;
	httpd_handle_t srv = NULL;
	ret = Web_wifi_init();

	srv = Web_http_init();

	return ret;

}


esp_err_t Web_wifi_init(void){
	 esp_err_t ret = nvs_flash_init();

	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase()); //potencjalnie niebezpieczne
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);


    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));


    wifi_config_t wifi_config = {
    	.ap =
    	{
    		.ssid = WIFI_SSID,
    		.ssid_len = strlen(WIFI_SSID),
    		.channel = WIFI_CHANNEL,
    		.password = WIFI_PASS,
    		.max_connection = MAX_STA_CONN,
    		.authmode = WIFI_AUTH_WPA_WPA2_PSK
    	},
    };


    if (strlen(WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Soft AP initialization finished. SSID: %s password: %s channel: %d",
    		WIFI_SSID, WIFI_PASS, WIFI_CHANNEL);

    tcpip_adapter_ip_info_t ip_info;
    ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &ip_info));

    ESP_LOGI(TAG,"IP Address:  %s", ip4addr_ntoa(&ip_info.ip));
    ESP_LOGI(TAG,"Subnet mask: %s", ip4addr_ntoa(&ip_info.netmask));
    ESP_LOGI(TAG,"Gateway:     %s", ip4addr_ntoa(&ip_info.gw));

    return ESP_OK;
}




esp_err_t index_html_get_handler(httpd_req_t *req)
{
    extern const unsigned char index_html_start[] asm("_binary_index_html_start");
    extern const unsigned char index_html_end[]   asm("_binary_index_html_end");
    const size_t index_html_size = (index_html_end - index_html_start);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start, index_html_size);
    return ESP_OK;
}

httpd_uri_t website_file = {
	.uri       = "/",
	.method    = HTTP_GET,
	.handler   = index_html_get_handler,
	.user_ctx  = NULL
};



httpd_handle_t Web_http_init(void){

	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

	ESP_LOGI(TAG, "Starting HTTP Server");
	if (httpd_start(&server, &config) != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to start HTTP server!");
		return 1;
	}
	ESP_LOGI(TAG, "Started HTTP server successfully");

	httpd_register_uri_handler(server, &website_file);

	return 0;
}

void Web_http_stop(httpd_handle_t server){
    if (server) {
        httpd_stop(server);
        ESP_LOGI(TAG, "Server stopped successfully");
    }
    else{
    	ESP_LOGW(TAG, "Server is already stopped");
    }
}










