#include "SBUS_driver.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "driver/uart.h"

#include "BOARD.h"

 uart_config_t uart_config = {
        .baud_rate = SBUS_SPEED,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

esp_err_t SBUS_begin(){
    esp_err_t ret = ESP_FAIL;

    ret = uart_param_config(UART_NUM_1, &uart_config);
    ret = uart_set_pin(UART_NUM_1, UART_EXT_OUT, UART_EXT_IN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    ret = uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0);

    return ret;
}

esp_err_t SBUS_receive(uint8_t *buf){
    uint32_t len = uart_read_bytes(UART_NUM_1, buf, BUF_SIZE, 20 / portTICK_RATE_MS);

    return ESP_OK;
}

esp_err_t SBUS_send(uint8_t *buf, uint32_t len){
    uart_write_bytes(UART_NUM_1, (const char *) buf, len);

    return ESP_OK;
}