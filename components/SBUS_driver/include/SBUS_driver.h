#pragma once

#include "esp_err.h"
#include "BOARD_cfg.h"
#include "driver/uart.h"

#define SBUS_CHANNELS 16
#define SBUS_SPEED 100000
#define BUF_SIZE 1024

typedef struct {
    int16_t min_value;  /*!< SBUS raw value at -100% position. */
    int16_t max_value;  /*!< SBUS raw value at +100% position. */
} SBUS_channel_config_t;

typedef struct  {
  bool lost_frame;
  bool failsafe;
  bool ch17, ch18;
  int16_t ch[SBUS_CHANNELS];
}SBUS_config_t;



esp_err_t SBUS_init();
esp_err_t SBUS_send(uint8_t *buf, uint32_t len);
esp_err_t SBUS_configChannel(uint8_t channel, int16_t min_value, int16_t max_value);
esp_err_t SBUS_setChannel(uint8_t channel, int8_t position);
esp_err_t SBUS_update();
