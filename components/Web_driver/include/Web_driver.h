#pragma once

#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_vfs.h"

#include "Web_driver_json.h"


/* Scratch buffer size */
#define SCRATCH_BUFSIZE  8192

struct file_server_data {
    /* Base path of file storage */
    char base_path[ESP_VFS_PATH_MAX + 1];

    /* Scratch buffer for temporary storage during file transfer */
    char scratch[SCRATCH_BUFSIZE];
};

esp_err_t Web_init(void);
esp_err_t Web_off(void);

void Web_status_exchange(Web_driver_status_t EX_status);
void Web_live_exchange(Web_driver_live_t EX_live);
