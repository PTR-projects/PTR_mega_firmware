#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*SPIFLASH_done_cb_t)(void);

esp_err_t SPIFLASH_init(void);
uint32_t  SPIFLASH_getSize(void);
bool      SPIFLASH_isBusy(void);
esp_err_t SPIFLASH_read(uint32_t position, void *buffer, uint32_t size);
esp_err_t SPIFLASH_prog(uint32_t position, void *buffer, uint32_t size);
esp_err_t SPIFLASH_eraseAll(SPIFLASH_done_cb_t done_cb);

#ifdef __cplusplus
}
#endif
