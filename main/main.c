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
#include "LED_driver.h"
#include "LORA_driver.h"
#include "GNSS_driver.h"
#include "Analog_driver.h"

#include "Sensors.h"
#include "AHRS_driver.h"
#include "FlightStateDetector.h"
#include "Data_aggregator.h"

#include "WiFi_driver.h"


//----------- Our defines --------------
#define ESP_CORE_0 0
#define ESP_CORE_1 1

static const char *TAG = "KP-PTR";

//----------- Queues etc ---------------
QueueHandle_t queue_AnalogStorage;

// periodic task with timer https://www.esp32.com/viewtopic.php?t=10280

void task_kpptr_main(void *pvParameter){
	TickType_t xLastWakeTime = 0;
	TickType_t prevTickCountRF = 0;
	DataPackage_t *  DataPackage_ptr = NULL;
	DataPackageRF_t  DataPackageRF_d;
	gps_t gps_d;
	Analog_meas_t Analog_meas;

	Sensors_init();
	GPS_init();
	LORA_init();
	//Detector_init();
	//AHRS_init();
	ESP_LOGI(TAG, "Task Main - ready!\n");

	xLastWakeTime = xTaskGetTickCount ();
	while(1){				//<<----- TODO zrobi� wyzwalanie z timera
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS( 100 ));

		struct timeval tv_now;
		gettimeofday(&tv_now, NULL);
		int64_t time_us = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;

		Sensors_update();
		//AHRS_calc();
		//Detector_detect();

		LED_blinkWS(0, COLOUR_AQUA, 20, 100, 1, 1);

		if(GPS_getData(&gps_d, 0))
			LED_blinkWS(1, COLOUR_ORANGE, 20, 100, 1, 1);

		xQueueReceive(queue_AnalogStorage, &Analog_meas, 0);

		Data_getFreePointerToMainRB(&DataPackage_ptr);
		Data_aggregate(DataPackage_ptr, time_us, Sensors_get(), &gps_d, NULL, NULL, NULL, &Analog_meas);
		Data_addToMainRB(&DataPackage_ptr);

		//send data to RF
		if(((prevTickCountRF + pdMS_TO_TICKS( 300 )) <= xLastWakeTime)){
			prevTickCountRF = xLastWakeTime;
			Data_aggregateRF(&DataPackageRF_d, time_us, Sensors_get(), &gps_d, NULL, NULL, NULL);
			LORA_sendPacketLoRa((uint8_t *)&DataPackageRF_d, sizeof(DataPackageRF_t), 0, 0);
			LED_blinkWS(2, COLOUR_PURPLE, 20, 100, 1, 1);
		}
	}
	vTaskDelete(NULL);
}

void task_kpptr_storage(void *pvParameter){
	DataPackage_t * DataPackage_ptr;
	while(1){
		if(0){	//(flightstate >= Launch) && (flightstate < Landed_delay)
			Data_getUsedPointerFromMainRB(&DataPackage_ptr);
			//save to flash
			Data_returnUsedPointerToMainRB(&DataPackage_ptr);
		}
	}
}

void task_kpptr_utils(void *pvParameter){
	TickType_t xLastWakeTime = 0;
	uint32_t interval_ms = 20;

	LED_init(interval_ms);
	BUZZER_init();
	ESP_LOGI(TAG, "Task Utils - ready!\n");

	xLastWakeTime = xTaskGetTickCount ();
	while(1){
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS( interval_ms ));
		LED_srv();
	}
	vTaskDelete(NULL);
}

void task_kpptr_analog(void *pvParameter){
	TickType_t xLastWakeTime = 0;
	uint32_t interval_ms = 100;

	Analog_meas_t Analog_meas;

	Analog_init(100);
	ESP_LOGI(TAG, "Task Analog - ready!\n");

	xLastWakeTime = xTaskGetTickCount ();
	while(1){
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS( interval_ms ));
		Analog_update(&Analog_meas);

		xQueueOverwrite(queue_AnalogStorage, (void *)&Analog_meas);
	}
	vTaskDelete(NULL);
}



void app_main(void)
{
    nvs_flash_init();
    WiFi_init();
    SPI_init(1000000);
    Data_init();

    //----- Create queues ----------
    queue_AnalogStorage = xQueueCreate( 1, sizeof( Analog_meas_t ) );

    //----- Check queues -----------
    if(queue_AnalogStorage == 0)
    	ESP_LOGE(TAG, "Failed to create queue -> queue_AnalogStorage");

    xTaskCreatePinnedToCore(&task_kpptr_utils, 		"task_kpptr_utils", 	1024*4, NULL, configMAX_PRIORITIES - 10, NULL, ESP_CORE_0);
    xTaskCreatePinnedToCore(&task_kpptr_analog, 	"task_kpptr_analog", 	1024*4, NULL, configMAX_PRIORITIES - 11, NULL, ESP_CORE_0);
    xTaskCreatePinnedToCore(&task_kpptr_storage,	"task_kpptr_storage",    024*4, NULL, configMAX_PRIORITIES - 3,  NULL, ESP_CORE_0);
    vTaskDelay(pdMS_TO_TICKS( 40 ));
    xTaskCreatePinnedToCore(&task_kpptr_main,		"task_kpptr_main",      1024*4, NULL, configMAX_PRIORITIES - 1,  NULL, ESP_CORE_1);



    while (true) {

        vTaskDelay(pdMS_TO_TICKS( 1000 ));
    }
}

