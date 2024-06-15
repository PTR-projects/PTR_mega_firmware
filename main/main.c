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
#include "Storage_driver.h"
#include "Sensors.h"
#include "AHRS_driver.h"
#include "FlightStateDetector.h"
#include "Web_driver.h"
#include "Preferences.h"
#include "DataManager.h"
#include "SysMgr.h"
#include "Servo_driver.h"

//----------- Our defines --------------
#define ESP_CORE_0 0
#define ESP_CORE_1 1

static const char *TAG = "KP-PTR";

//----------- Queues etc ---------------
QueueHandle_t queue_AnalogToMain;
QueueHandle_t queue_MainToTelemetry;
QueueHandle_t queue_MainToWeb;

/**
 * @brief Main Task with sensors handling, data managment, AHRS calculations
 * and Flight State Detection
 *
 * @param pvParameter
 */
void task_kpptr_main(void *pvParameter){
	TickType_t 		 xLastWakeTime = 0;
	TickType_t 		 prevTickCountRF = 0;
	TickType_t 		 prevTickCountWeb = 0;
	DataPackage_t *  DataPackage_ptr = NULL;
	DataPackage_t    DataPackage_d;
	DataPackageRF_t  DataPackageRF_d;
	gps_t 			 gps_d;
	Analog_meas_t 	 Analog_meas;

	int64_t time_us = esp_timer_get_time();

	esp_err_t status = ESP_FAIL;
	while(status != ESP_OK){
		status  = ESP_OK;
		status |= Sensors_init();
		status |= GPS_init();
		status |= AHRS_init(time_us);
		status |= FSD_init(AHRS_getData());

		if(status != ESP_OK){
			ESP_LOGW(TAG, "Main task - failed to prepare main task");
			SysMgr_checkout(checkout_main, check_fail);
			vTaskDelay(pdMS_TO_TICKS( 1000 ));
		}
	} 

	SysMgr_checkout(checkout_main, check_ready);
	ESP_LOGI(TAG, "Task Main - ready!");

	xLastWakeTime = xTaskGetTickCount ();
	while(1){
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS( 10 ));	// Note - for rate > 100Hz change MS5607 settings

		int64_t time_us = esp_timer_get_time();

		Sensors_update();
		AHRS_compute(time_us, Sensors_get());
		GPS_getData(&gps_d, 0);
		FSD_detect(time_us/1000);

		xQueueReceive(queue_AnalogToMain, &Analog_meas, 0);

		DM_collectFlash(&DataPackage_d, time_us, Sensors_get(), &gps_d, AHRS_getData(), FSD_getState(), NULL, &Analog_meas, Servo_get());

		if(DM_getFreePointerToMainRB(&DataPackage_ptr) == ESP_OK){
			if(DataPackage_ptr != NULL){
				*DataPackage_ptr = DataPackage_d;
				DM_addToMainRB(&DataPackage_ptr);

			} else {
				ESP_LOGE(TAG, "Main RB pointer = NULL!");
			}
		} else {
			ESP_LOGE(TAG, "Main RB error!");
		}

#if defined (RF_BUSY_PIN) && defined (RF_RST_PIN) && defined (SPI_SLAVE_SX1262_PIN)
		//send data to RF every 1000ms
		if(((prevTickCountRF + pdMS_TO_TICKS( 1000 )) <= xLastWakeTime)){
			prevTickCountRF = xLastWakeTime;
			DM_collectRF(&DataPackageRF_d, time_us, Sensors_get(), &gps_d, AHRS_getData(), FSD_getState(), NULL);
			xQueueOverwrite(queue_MainToTelemetry, (void *)&DataPackageRF_d); // add to telemetry queue
		}
#endif

		//send data to Web every 1000ms
		if(((prevTickCountWeb + pdMS_TO_TICKS( 1000 )) <= xLastWakeTime)){
			prevTickCountWeb = xLastWakeTime;
			xQueueOverwrite(queue_MainToWeb, (void *)DataPackage_ptr); // add to Web queue
		}

	}
	vTaskDelete(NULL);
}

/**
 * @brief Task dedicatet to telemetry handling
 *
 * @param pvParameter
 */
void task_kpptr_telemetry(void *pvParameter){
#if defined (RF_BUSY_PIN) && defined (RF_RST_PIN) && defined (SPI_SLAVE_SX1262_PIN)
	DataPackageRF_t DataPackageRF_d;
	while(LORA_init() != ESP_OK){
		ESP_LOGW(TAG, "Telemetry task - failed to prepare Lora");
		SysMgr_checkout(checkout_lora, check_fail);
		vTaskDelay(pdMS_TO_TICKS( 1000 ));
	}

	SysMgr_checkout(checkout_lora, check_ready);
	while(1){
		if(xQueueReceive(queue_MainToTelemetry, &DataPackageRF_d, 100)){
			LORA_sendPacketLoRa((uint8_t *)&DataPackageRF_d, sizeof(DataPackageRF_t), LORA_TX_NO_WAIT);
		}
	}
#else
	SysMgr_checkout(checkout_lora, check_ready);
	while(1){
		vTaskDelay(pdMS_TO_TICKS( 1000 ));
	}
#endif
}

/**
 * @brief Task that manages data storage
 *
 * @param pvParameter
 */
void task_kpptr_storage(void *pvParameter){
	TickType_t xLastWakeTime = 0;
	while(Storage_init() != ESP_OK){
		ESP_LOGW(TAG, "Storage task - failed to prepare storage");
		SysMgr_checkout(checkout_storage, check_void);
		vTaskDelay(pdMS_TO_TICKS( 3000 ));
	}

	DataPackage_t * DataPackage_ptr;
	uint32_t write_error_cnt = 0;

	vTaskDelay(pdMS_TO_TICKS( 2000 ));
	ESP_LOGI(TAG, "Task Storage - ready!");
	SysMgr_checkout(checkout_storage, check_ready);

	xLastWakeTime = xTaskGetTickCount ();
	while(1){
		// Minimum 2 Ticks for 1 loop - avoid blocking Flash memory for too long
		vTaskDelayUntil(&xLastWakeTime, 2);

		// Skip if we are not flying yet
		if((FSD_getState() < FLIGHTSTATE_ME_ACCELERATING) && (FSD_getState() >= FLIGHTSTATE_SHUTDOWN))
				continue;

		// Skip if timeout occured (max 100ms)
		if(DM_getUsedPointerFromMainRB_wait(&DataPackage_ptr) != ESP_OK){
			ESP_LOGI(TAG, "Storage timeout");
			continue;
		}

		// Skip if too many write errors occured but free used pointer
		if(write_error_cnt > 1000){
			DM_returnUsedPointerToMainRB(&DataPackage_ptr);
			continue;
		}

		// Try write data to Flash
		if(Storage_writePacket((void*)DataPackage_ptr, sizeof(DataPackage_t)) != ESP_OK){
			ESP_LOGE(TAG, "Storage task - packet write fail");
			write_error_cnt++;
		} else {
			write_error_cnt = 0;	// Reset error counter if write successful
		}
	}
}

/**
 * @brief Utility task for low priority functions
 *
 * @param pvParameter
 */
void task_kpptr_utils(void *pvParameter){
	TickType_t 	  xLastWakeTime = 0;
	uint32_t 	  interval_ms = 20;
	DataPackage_t DataPackage_d;
	esp_err_t 	  status = ESP_FAIL;

	while(status != ESP_OK){
		status  = ESP_OK;
		status |= LED_init(interval_ms);
		status |= BUZZER_init();
		status |= IGN_init();

		if(status != ESP_OK){
			ESP_LOGW(TAG, "Task Utils - failed to init!");
			SysMgr_checkout(checkout_utils, check_fail);
			vTaskDelay(pdMS_TO_TICKS( 1000 ));
		}
	}

	ESP_LOGI(TAG, "Task Utils - ready!");

#if !defined GNSS_UART
	SysMgr_checkout(checkout_gnss, check_ready);
#endif

	SysMgr_checkout(checkout_utils, check_ready);
	xLastWakeTime = xTaskGetTickCount ();
	
	while(1){
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS( interval_ms ));
		LED_srv();
		IGN_srv(pdTICKS_TO_MS(xTaskGetTickCount ()));

		if(xQueueReceive(queue_MainToWeb, &DataPackage_d, 0)){
			Web_live_from_DataPackage(&DataPackage_d);

#if defined GNSS_UART
			// change GNSS component status if fix is OK
			if(GPS_checkStatus() == ESP_OK){
				static sysmgr_checkout_state_t gnss_ready_check = check_void;
				if((gnss_ready_check == check_void)
						/*&& (DataPackage_ptr->sensors.gnss_fix)*/){
					gnss_ready_check = check_ready;
					SysMgr_checkout(checkout_gnss, check_ready);
				}
			} else {
				SysMgr_checkout(checkout_gnss, check_fail);
			}
#endif

		}
	}
	vTaskDelete(NULL);
}

/**
 * @brief Task handling Analog measurements
 *
 * @param pvParameter
 */
void task_kpptr_analog(void *pvParameter){
	TickType_t 				xLastWakeTime = 0;
	uint32_t 				interval_ms = 100;
	sysmgr_checkout_state_t vbat_ok = check_void;
	Analog_meas_t 			Analog_meas;

	while(Analog_init(100, 0.2f) != ESP_OK){
		ESP_LOGW(TAG, "Task Analog - failed to init!");
		SysMgr_checkout(checkout_analog, check_fail);
		vTaskDelay(pdMS_TO_TICKS( 1000 ));
	}
	ESP_LOGI(TAG, "Task Analog - ready!");

	xLastWakeTime = xTaskGetTickCount ();
	while(1){
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS( interval_ms ));
		Analog_update(&Analog_meas);
		Web_status_updateAnalog(Analog_meas.vbat_mV/1000.0f,
								Analog_getIGNstate(&Analog_meas, 0), Analog_getIGNstate(&Analog_meas, 1),
								Analog_getIGNstate(&Analog_meas, 2), Analog_getIGNstate(&Analog_meas, 3));

		if((vbat_ok != check_ready) && (Analog_meas.vbat_mV > 3700.0f)){
			vbat_ok = check_ready;
			SysMgr_checkout(checkout_analog, check_ready);
		}

		for(uint8_t i=0; i<IGN_NUM; i++){
			LED_setIGN(i, 20, Analog_meas.IGN_det[i]);
		}

		xQueueOverwrite(queue_AnalogToMain, (void *)&Analog_meas);
	}
	vTaskDelete(NULL);
}

/**
 * @brief System Manager Task - checks components health
 *
 * @param pvParameter
 */
void task_kpptr_sysmgr(void *pvParameter){
	int64_t ready_to_arm_time = 0;
	ESP_LOGI(TAG, "SysMgr ready");
	SysMgr_checkout(checkout_sysmgr, check_ready);

	while(1){
		SysMgr_update();	//Process new messages

		switch(SysMgr_getCheckoutStatus()){
		case check_ready:
			LED_blinkWS(LED_STAT, COLOUR_GREEN, 20, 100, 0, 0);
			break;

		case check_void:
			LED_blinkWS(LED_STAT, COLOUR_ORANGE, 20, 100, 0, 0);
			break;

		case check_fail:
			LED_blinkWS(LED_STAT, COLOUR_RED, 20, 300, 300, 0);
			break;

		default:
			break;
		}

		switch(SysMgr_getArm()){
		case system_armed:
			LED_blinkWS(LED_ARM, COLOUR_GREEN, 20, 100, 0, 0);
			break;

		case system_dissarmed:
			LED_blinkWS(LED_ARM, COLOUR_ORANGE, 20, 100, 0, 0);
			break;

		case system_arming_error:
			LED_blinkWS(LED_ARM, COLOUR_RED, 20, 300, 300, 0);
			break;

		default:
			break;
		}

		Web_status_updateSysMgr(esp_timer_get_time()/1000, 	SysMgr_getCheckoutStatus(), 	SysMgr_getComponentState(checkout_analog),
												SysMgr_getComponentState(checkout_lora), 	SysMgr_getComponentState(checkout_main),
												SysMgr_getComponentState(checkout_storage), SysMgr_getComponentState(checkout_sysmgr),
												SysMgr_getComponentState(checkout_utils), 	SysMgr_getComponentState(checkout_web),
												SysMgr_getArm());

		//--------------- Autoarming ----------------------------
		if(FSD_checkArmed() == DISARMED){
			if(SysMgr_getCheckoutStatus() == check_ready){
				if(ready_to_arm_time == 0){
					ready_to_arm_time = esp_timer_get_time();
				}
				if((esp_timer_get_time() - ready_to_arm_time) > 15000000UL){
					FSD_arming();
					if(FSD_checkArmed() == ARMED){
						SysMgr_setArm(system_armed);
						BUZZER_beep(70, 70, 5);
					}
				}
			}
		}

		vTaskDelay(pdMS_TO_TICKS( 100 ));	// Limit loop rate to max 10Hz
	}
}

/**
 * @brief Main entry task
 *
 */
void app_main(void)
{
    nvs_flash_init();
    Web_storageInit();
    Preferences_init();
    SysMgr_init();

    // Init Web component and make checkout
    if(Web_init() == ESP_OK){
    	SysMgr_checkout(checkout_web, check_ready);
    } else {
    	SysMgr_checkout(checkout_web, check_fail);
    }

    SPI_init();
    DM_init();

    //-----
    Preferences_data_t pref;
	if(Preferences_get(&pref) == ESP_OK){
		Web_status_updateconfig(0, 12345, pref.drouge_alt_m, pref.main_alt_m);
	}

    //----- Create queues ----------
    queue_AnalogToMain    = xQueueCreate( 1, sizeof( Analog_meas_t   ) );
    queue_MainToTelemetry = xQueueCreate( 1, sizeof( DataPackageRF_t ) );
    queue_MainToWeb 	  = xQueueCreate( 1, sizeof( DataPackage_t   ) );

    //----- Check queues -----------
    if(queue_AnalogToMain == 0)
    	ESP_LOGE(TAG, "Failed to create queue -> queue_AnalogToMain");

    xTaskCreatePinnedToCore(&task_kpptr_sysmgr, 	"task_kpptr_sysmgr", 	1024*4, NULL, configMAX_PRIORITIES - 10, NULL, ESP_CORE_0);
    xTaskCreatePinnedToCore(&task_kpptr_utils, 		"task_kpptr_utils", 	1024*4, NULL, configMAX_PRIORITIES - 14, NULL, ESP_CORE_0);
    xTaskCreatePinnedToCore(&task_kpptr_analog, 	"task_kpptr_analog", 	1024*4, NULL, configMAX_PRIORITIES - 13, NULL, ESP_CORE_0);
    xTaskCreatePinnedToCore(&task_kpptr_storage,	"task_kpptr_storage",   1024*4, NULL, configMAX_PRIORITIES - 3,  NULL, ESP_CORE_0);
    xTaskCreatePinnedToCore(&task_kpptr_telemetry,	"task_kpptr_telemetry", 1024*4, NULL, configMAX_PRIORITIES - 4,  NULL, ESP_CORE_0);
    vTaskDelay(pdMS_TO_TICKS( 40 ));
    xTaskCreatePinnedToCore(&task_kpptr_main,		"task_kpptr_main",      1024*4, NULL, configMAX_PRIORITIES - 1,  NULL, ESP_CORE_1);

    while (true) {
    	vTaskDelay(pdMS_TO_TICKS( 1000 ));	// Limit loop rate to max 1Hz
    }
}
