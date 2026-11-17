#pragma once

#include "esp_err.h"

/**
 * @brief Mount SD card (FAT) if SD_ENABLED; otherwise a no-op.
 */
esp_err_t SD_init(void);

/**
 * @brief Export SimpleFS flight log to next NNN.bin and NNN.csv on SD.
 *        Requires SD_init() success when SD_ENABLED.
 */
esp_err_t SD_exportFlightLog(void);
