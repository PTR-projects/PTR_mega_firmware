#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "driver/uart.h"

#include "BOARD_cfg.h"
#include "SBUS_driver.h"

static const char *TAG = "SBUS";

static SBUS_config_t         SBUS_config_d;
static SBUS_channel_config_t SBUS_channel_config_d[SBUS_CHANNELS];

static int16_t position_to_SBUS(int8_t position, SBUS_channel_config_t config){
    return (int16_t)((position + 100) * (config.max_value - config.min_value) / 200 + config.min_value);
}

static const uart_config_t uart_config = {
        .baud_rate = SBUS_SPEED,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_EVEN,
        .stop_bits = UART_STOP_BITS_2,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

esp_err_t SBUS_init(){
    memset(&SBUS_config_d, 0, sizeof(SBUS_config_d));

    for(int i = 0; i < SBUS_CHANNELS; i++){
        SBUS_channel_config_d[i].min_value = 172;
        SBUS_channel_config_d[i].max_value = 1811;
    }

    ESP_RETURN_ON_ERROR(uart_param_config(UART_EXT_UART, &uart_config), TAG, "uart_param_config failed");
    ESP_RETURN_ON_ERROR(uart_set_pin(UART_EXT_UART, UART_EXT_OUT, UART_EXT_IN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE), TAG, "uart_set_pin failed");
    ESP_RETURN_ON_ERROR(uart_driver_install(UART_EXT_UART, BUF_SIZE * 2, 0, 0, NULL, 0), TAG, "uart_driver_install failed");
    ESP_RETURN_ON_ERROR(uart_set_line_inverse(UART_EXT_UART, UART_SIGNAL_TXD_INV), TAG, "uart inverse mode failed");

    return ESP_OK;
}

esp_err_t SBUS_send(uint8_t *buf, uint32_t len){
    uart_write_bytes(UART_NUM_1, (const char *) buf, len);

    return ESP_OK;
}

esp_err_t SBUS_configChannel(uint8_t channel, int16_t min_value, int16_t max_value){
    ESP_RETURN_ON_FALSE(channel < SBUS_CHANNELS, ESP_ERR_INVALID_ARG, TAG, "channel out of range");

    SBUS_channel_config_d[channel].min_value = min_value;
    SBUS_channel_config_d[channel].max_value = max_value;

    return ESP_OK;
}

esp_err_t SBUS_setChannel(uint8_t channel, int8_t position){
    ESP_RETURN_ON_FALSE(channel < SBUS_CHANNELS, ESP_ERR_INVALID_ARG, TAG, "channel out of range");

    SBUS_config_d.ch[channel] = position_to_SBUS(position, SBUS_channel_config_d[channel]);

    return ESP_OK;
}

esp_err_t SBUS_update(){
    uint8_t buf[25];

    buf[0]  = 0x0F;

    // Pack 16 channels × 11 bits LSB-first into bytes 1–22
    buf[1]  = (uint8_t)( SBUS_config_d.ch[0]        & 0x07FF);
    buf[2]  = (uint8_t)((SBUS_config_d.ch[0] >> 8   & 0x07FF) | (SBUS_config_d.ch[1]  << 3  & 0xFF));
    buf[3]  = (uint8_t)((SBUS_config_d.ch[1] >> 5   & 0x07FF) | (SBUS_config_d.ch[2]  << 6  & 0xFF));
    buf[4]  = (uint8_t)( SBUS_config_d.ch[2] >> 2   & 0x07FF);
    buf[5]  = (uint8_t)((SBUS_config_d.ch[2] >> 10  & 0x07FF) | (SBUS_config_d.ch[3]  << 1  & 0xFF));
    buf[6]  = (uint8_t)((SBUS_config_d.ch[3] >> 7   & 0x07FF) | (SBUS_config_d.ch[4]  << 4  & 0xFF));
    buf[7]  = (uint8_t)((SBUS_config_d.ch[4] >> 4   & 0x07FF) | (SBUS_config_d.ch[5]  << 7  & 0xFF));
    buf[8]  = (uint8_t)( SBUS_config_d.ch[5] >> 1   & 0x07FF);
    buf[9]  = (uint8_t)((SBUS_config_d.ch[5] >> 9   & 0x07FF) | (SBUS_config_d.ch[6]  << 2  & 0xFF));
    buf[10] = (uint8_t)((SBUS_config_d.ch[6] >> 6   & 0x07FF) | (SBUS_config_d.ch[7]  << 5  & 0xFF));
    buf[11] = (uint8_t)( SBUS_config_d.ch[7] >> 3   & 0x07FF);
    buf[12] = (uint8_t)( SBUS_config_d.ch[8]        & 0x07FF);
    buf[13] = (uint8_t)((SBUS_config_d.ch[8] >> 8   & 0x07FF) | (SBUS_config_d.ch[9]  << 3  & 0xFF));
    buf[14] = (uint8_t)((SBUS_config_d.ch[9] >> 5   & 0x07FF) | (SBUS_config_d.ch[10] << 6  & 0xFF));
    buf[15] = (uint8_t)( SBUS_config_d.ch[10] >> 2  & 0x07FF);
    buf[16] = (uint8_t)((SBUS_config_d.ch[10] >> 10 & 0x07FF) | (SBUS_config_d.ch[11] << 1  & 0xFF));
    buf[17] = (uint8_t)((SBUS_config_d.ch[11] >> 7  & 0x07FF) | (SBUS_config_d.ch[12] << 4  & 0xFF));
    buf[18] = (uint8_t)((SBUS_config_d.ch[12] >> 4  & 0x07FF) | (SBUS_config_d.ch[13] << 7  & 0xFF));
    buf[19] = (uint8_t)( SBUS_config_d.ch[13] >> 1  & 0x07FF);
    buf[20] = (uint8_t)((SBUS_config_d.ch[13] >> 9  & 0x07FF) | (SBUS_config_d.ch[14] << 2  & 0xFF));
    buf[21] = (uint8_t)((SBUS_config_d.ch[14] >> 6  & 0x07FF) | (SBUS_config_d.ch[15] << 5  & 0xFF));
    buf[22] = (uint8_t)( SBUS_config_d.ch[15] >> 3  & 0x07FF);

    // Flags byte
    buf[23] = (SBUS_config_d.ch17       ? 0x01 : 0x00)
            | (SBUS_config_d.ch18       ? 0x02 : 0x00)
            | (SBUS_config_d.lost_frame ? 0x04 : 0x00)
            | (SBUS_config_d.failsafe   ? 0x08 : 0x00);

    buf[24] = 0x00;

    return SBUS_send(buf, sizeof(buf));
}