#pragma once

esp_err_t IGN_init(void);
esp_err_t IGN_srv(uint32_t time);
uint8_t IGN_getState(uint8_t ign_no);
esp_err_t IGN_set(uint8_t ign_no, uint8_t state);
uint8_t IGN_check(uint8_t ign_no);
