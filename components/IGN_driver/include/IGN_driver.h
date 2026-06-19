#pragma once

#include "esp_err.h"

typedef struct{

} IGN_t;

/**
 * @brief Initialize igniter component by configuring IO pins
 */
esp_err_t IGN_init(void);

/**
 * @brief Function to ensure that igniters turn off after specific time
 *
 * @param time system time in ms
 */
esp_err_t IGN_srv(uint32_t time);

/**
 * @brief Get state of selected igniter 
 *
 * @param ign_no igniter ID
 * @return int8_t
 *  - 1: igniter is ON
 *  - 0: igniter is OFF
 *  - -1: failed to get state
 */
int8_t IGN_getState(uint8_t ign_no);

/**
 * @brief Select state of selected igniter
 *
 * @param ign_no igniter ID
 * @param state state of selected igniter
 *  - 1: set igniter to ON state
 *  - 0: set igniter to OFF state
 * @return esp_err_t
 *  - ESP_OK: Success
 *  - Others: Fail
 */
esp_err_t IGN_set(uint8_t ign_no, uint8_t state);

/**
 * @brief Placeholder
 *
 * @param ign_no igniter ID
 */
uint8_t IGN_check(uint8_t ign_no);
