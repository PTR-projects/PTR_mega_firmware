#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_event.h"

#include "cJSON.h"


#include "lwip/err.h"
#include "lwip/sys.h"

#include "Web_driver.h"
#include "Web_driver_json.h"

static const char *TAG = "Web_driver_cmd";

/*!
 * @brief Handle commands from sjon string.
 * @return `ESP_OK` if initialized
 * @return `ESP_FAIL` otherwise.
 */
esp_err_t Web_cmd_handler(char *buf){
	cJSON *json = cJSON_Parse(buf);
	ESP_LOGI(TAG, "%s", buf);


	char *cmd = cJSON_GetObjectItem(json, "cmd")->valuestring;
	ESP_LOGI(TAG, "Command: %s", cmd);
	uint32_t key =  cJSON_GetObjectItem(json, "key")->valueint;
	ESP_LOGI(TAG, "Key: %d", key);

	if(strcmp(cmd,"ign_set") == 0){
		ESP_LOGI(TAG, "Igniter fire!");
	}


	cJSON_Delete(json);

	return ESP_OK;

}
