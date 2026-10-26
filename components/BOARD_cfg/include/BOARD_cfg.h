#pragma once

// Global configs
#define CONFIG_ESP_WIFI_PASSWORD "MeteorPTR"

#define CONFIG_KPPTR_MASTERKEY 2137

#define CONFIG_FS_SIMPLEFS

#define CONFIG_NMEA_PARSER_TASK_STACK_SIZE 2048
#define CONFIG_NMEA_PARSER_TASK_PRIORITY 2
#define CONFIG_NMEA_PARSER_RING_BUFFER_SIZE 1024
#define CONFIG_NMEA_STATEMENT_GGA 1

#if defined(CONFIG_BOARD_PTR_MEGA_VER_0_REV_1)
#include "boards/board_ptr_mega_ver_0_rev_1.h"
#elif defined(CONFIG_BOARD_PTR_MEGA_VER_1_REV_0)
#include "boards/board_ptr_mega_ver_1_rev_0.h"
#elif defined(CONFIG_BOARD_ARECORDER_VER_3_REV_0)
#include "boards/board_arecorder_ver_3_rev_0.h"
#elif defined(CONFIG_BOARD_PTR_MINI_VER_1_REV_0)
#include "boards/board_ptr_mini_ver_1_rev_0.h"
#elif defined(CONFIG_BOARD_PTR_MICRO_VER_0_REV_1)
#include "boards/board_ptr_micro_ver_0_rev_1.h"
#else
#error "This board version don't exist or require new definition."
#endif

#ifndef LED_POS_BAT
#define LED_POS_BAT -1
#endif
