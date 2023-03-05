#pragma once

typedef struct {
	uint32_t key;
} Web_driver_cmd_t;

esp_err_t Web_cmd_handler(char *buf);
esp_err_t Web_cmd_init(uint32_t key);
