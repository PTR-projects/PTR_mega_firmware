#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

//----------- Our includes --------------
#include "SPI_driver.h"

#include "Sensors.h"
#include "AHRS_driver.h"
#include "FlightStateDetector.h"
#include "WiFi_driver.h"
#include "UART_console.h"

//----------- Our defines --------------
#define ESP_CORE_0 0
#define ESP_CORE_1 1

static const char *TAG = "KP-PTR";

// periodic task with timer https://www.esp32.com/viewtopic.php?t=10280

void task_kpptr_main(void *pvParameter){
	Sensors_init();
	//Detector_init();
	//AHRS_init();

	while(1){				//<<----- TODO zrobi� wyzwalanie z timera
		Sensors_update();
		//AHRS_calc();
		//Detector_detect();
		//send data to logging task
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
	vTaskDelete(NULL);
}

void task_kpptr_logging(void *pvParameter){
	//SDinit();
	//FAT_init();
	//FLASH_init();

	while(1){
		//wait for data from main task
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
	vTaskDelete(NULL);
}

void task_kpptr_telemetry(void *pvParameter){
	//SDinit();
	SPI_init(1000000);
	//FAT_init();
	//FLASH_init();

	while(1){
		//wait for data from main task
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
	vTaskDelete(NULL);
}



void app_main(void)
{
    nvs_flash_init();
    //WiFi_init();
    UART_console_init();

//    xTaskCreatePinnedToCore(&task_kpptr_main,      "task_kpptr_main",      1024*4, NULL, configMAX_PRIORITIES - 1, NULL, ESP_CORE_0);
//    xTaskCreatePinnedToCore(&task_kpptr_logging,   "task_kpptr_logging",   1024*4, NULL, configMAX_PRIORITIES - 1, NULL, ESP_CORE_1);
//    xTaskCreatePinnedToCore(&task_kpptr_telemetry, "task_kpptr_telemetry", 1024*4, NULL, configMAX_PRIORITIES - 1, NULL, ESP_CORE_1);


    while (true) {

        vTaskDelay(300 / portTICK_PERIOD_MS);
    }
}

