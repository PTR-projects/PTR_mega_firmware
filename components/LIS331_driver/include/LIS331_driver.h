#pragma once

typedef enum{
	LIS331_IC,
	LIS331HH_IC,
	H3LIS331_IC
}LIS331_type_t;

typedef struct{

} LIS331_t;

esp_err_t LIS331_init(LIS331_type_t type);
