#pragma once

#include "esp_err.h"
#include "BOARD.h"
#include "driver/uart.h"

#define SBUS_CHANNELS 16
#define SBUS_SPEED 100000
#define BUF_SIZE 1024

typedef struct  {
  bool lost_frame;
  bool failsafe;
  bool ch17, ch18;
  int16_t ch[SBUS_CHANNELS];
}SBUS_config_t;



esp_err_t SBUS_begin();
esp_err_t SBUS_receive();
esp_err_t SBUS_send();

