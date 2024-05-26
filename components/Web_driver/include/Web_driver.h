#pragma once

#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_vfs.h"
#include "DataManager.h"
#include "Web_driver_json.h"


/* Scratch buffer size */
#define SCRATCH_BUFSIZE  1024

struct file_server_data {
    /* Base path of file storage */
    char base_path[ESP_VFS_PATH_MAX + 1];

    /* Scratch buffer for temporary storage during file transfer */
    char scratch[SCRATCH_BUFSIZE];
};

/*!
 * @brief Initialize web component by calling init functions for wifi and http server.
 * @return `ESP_OK` if initialized
 * @return `ESP_ERR_NOT_FOUND` if partition is not present
 * @return `ESP_FAIL` otherwise.
 */
esp_err_t Web_init(void);
esp_err_t Web_storageInit();
esp_err_t Web_off(void);

void Web_status_exchange(Web_driver_status_t EX_status);
void Web_live_exchange(Web_driver_live_t EX_live);

esp_err_t Web_status_updateAnalog(float vbat, uint8_t ign1_cont, uint8_t ign2_cont, uint8_t ign3_cont, uint8_t ign4_cont);
esp_err_t Web_status_updateIgniters(uint8_t ign1_fired, uint8_t ign2_fired, uint8_t ign3_fired, uint8_t ign4_fired);
esp_err_t Web_status_updateSysMgr(uint32_t timestamp_ms, uint8_t state_system, uint8_t state_analog, uint8_t state_lora,
								  uint8_t state_adcs, uint8_t state_storage, uint8_t state_sysmgr, uint8_t state_utils,
								  uint8_t state_web, uint8_t arm);
esp_err_t Web_status_updateconfig(uint64_t SWversion, uint64_t serialNumber, float drougeAlt, float mainAlt); //zakładam wykonywanie tego przy okazji odczyty konfiguracji konfiguracji, czyli na starcie i po zmienie konfiguracji
esp_err_t Web_status_updateGNSS(float lat, float lon, uint8_t fix, uint8_t sats);
esp_err_t Web_live_from_DataPackage(DataPackage_t * DataPackage_ptr);
esp_err_t Web_status_updateADCS(uint8_t flightstate, float rocket_tilt); //ADCS = Attitude Determination and Control System

