#pragma once

#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_vfs.h"


/* Scratch buffer size */
#define SCRATCH_BUFSIZE  8192

struct file_server_data {
    /* Base path of file storage */
    char base_path[ESP_VFS_PATH_MAX + 1];

    /* Scratch buffer for temporary storage during file transfer */
    char scratch[SCRATCH_BUFSIZE];
};

typedef struct{
	bool IGN_status[5];
} Rocket_status_t;

esp_err_t Web_init(void);
esp_err_t Web_off(void);
