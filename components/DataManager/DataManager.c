#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "BOARD.h"
#include "DataManager.h"
#define DA_MAIN_QUEUE_SIZE 100

//--------------- Main Data Buffer ----------------
static DataPackage_t DataPackage_rb[DA_MAIN_QUEUE_SIZE];

//--------------- QUEUE variables ------------------
QueueHandle_t  queue_StorageFree;
QueueHandle_t  queue_StorageUsed;

static StaticQueue_t  queue_StorageFree_struct;
static StaticQueue_t  queue_StorageUsed_struct;

static uint8_t queue_StorageFree_buf[ DA_MAIN_QUEUE_SIZE * sizeof(&DataPackage_rb) ];
static uint8_t queue_StorageUsed_buf[ DA_MAIN_QUEUE_SIZE * sizeof(&DataPackage_rb) ];

//--------------- Misc variables ----------------------
static const char *TAG = "Data ag.";
static uint16_t packet_counter = 0;

esp_err_t DM_init(){
	memset(DataPackage_rb, 0, sizeof(DataPackage_rb));

	//----- Create queues ----------
	queue_StorageFree = xQueueCreateStatic( DA_MAIN_QUEUE_SIZE,
							sizeof(&DataPackage_rb[0]),
							queue_StorageFree_buf,
							&queue_StorageFree_struct);
	queue_StorageUsed = xQueueCreateStatic( DA_MAIN_QUEUE_SIZE,
							sizeof(&DataPackage_rb[0]),
							queue_StorageUsed_buf,
							&queue_StorageUsed_struct);

	//----- Check queues -----------
	if(queue_StorageFree == 0){
		ESP_LOGE(TAG, "Failed to create queue -> queue_StorageFree");
		return ESP_FAIL;
	}
	if(queue_StorageUsed == 0){
		ESP_LOGE(TAG, "Failed to create queue -> queue_StorageUsed");
		return ESP_FAIL;
	}

	//----- Init queue -------------
	for(uint8_t i=0; i<DA_MAIN_QUEUE_SIZE; i++){
		DataPackage_t * DataPackage_ptr = &(DataPackage_rb[i]);
		if(xQueueSend(queue_StorageFree, &DataPackage_ptr, 100) != pdTRUE){
			ESP_LOGE(TAG, "Faild to add to Main RB!");
			return ESP_FAIL;
		}
	}

	ESP_LOGI(TAG, "RB init done");
	return ESP_OK;
}

//-------------------------- Unload from Main RB ---------------------------
esp_err_t DM_getUsedPointerFromMainRB(DataPackage_t ** ptr){
	if(xQueueReceive(queue_StorageUsed, ptr, 0) != pdTRUE)
		return ESP_FAIL;

	return ESP_OK;
}

esp_err_t DM_getUsedPointerFromMainRB_wait(DataPackage_t ** ptr){
	if(xQueueReceive(queue_StorageUsed, ptr, pdMS_TO_TICKS( 100 )) != pdTRUE)
		return ESP_FAIL;

	return ESP_OK;
}

esp_err_t DM_returnUsedPointerToMainRB(DataPackage_t ** ptr){
	if(xQueueSend(queue_StorageFree, ptr, 0) != pdTRUE)
			return ESP_FAIL;

	return ESP_OK;
}

//--------------------------- Loading to Main RB --------------------------
esp_err_t DM_getFreePointerToMainRB(DataPackage_t ** ptr){
	if(xQueueReceive(queue_StorageFree, ptr, 0) != pdTRUE){
		//no free item in Free Queue get oldest from Used Queue
		if(xQueueReceive(queue_StorageUsed, ptr, 0) != pdTRUE)
				return ESP_FAIL;
	}

	return ESP_OK;
}

esp_err_t DM_addToMainRB(DataPackage_t ** ptr){
	if(xQueueSend(queue_StorageUsed, ptr, 0) != pdTRUE)
		return ESP_FAIL;

	return ESP_OK;
}

void DM_collectFlash(DataPackage_t * package, int64_t time_us, Sensors_t * sensors, gps_t * gps, AHRS_t * ahrs,
		FlightState_t * flightstate, IGN_t * ign, Analog_meas_t * analog){

	package->sys_time = time_us;

	package->sensors.accHX = sensors->LIS331.accX;
	package->sensors.accHY = sensors->LIS331.accY;
	package->sensors.accHZ = sensors->LIS331.accZ;

	package->sensors.accX = sensors->LSM6DSO32.accX;
	package->sensors.accY = sensors->LSM6DSO32.accY;
	package->sensors.accZ = sensors->LSM6DSO32.accZ;

	package->sensors.gyroX = sensors->LSM6DSO32.gyroX;
	package->sensors.gyroY = sensors->LSM6DSO32.gyroY;
	package->sensors.gyroZ = sensors->LSM6DSO32.gyroZ;

	package->sensors.magX = sensors->MMC5983MA.magX;
	package->sensors.magY = sensors->MMC5983MA.magY;
	package->sensors.magZ = sensors->MMC5983MA.magZ;

	package->sensors.pressure = sensors->MS5607.press;
	package->sensors.temp = sensors->MS5607.temp;

	package->sensors.latitude = gps->latitude;
	package->sensors.longitude = gps->longitude;
	package->sensors.altitude_gnss = gps->altitude;
	package->sensors.gnss_fix = (int8_t)(gps->fix);

	package->ign.ign1_cont = analog->IGN1_det;
	package->ign.ign2_cont = analog->IGN2_det;
	package->ign.ign3_cont = analog->IGN3_det;
	package->ign.ign4_cont = analog->IGN4_det;

	package->vbat_mV = analog->vbat_mV;


	//package->flightstate = (uint8_t)(flightstate->state);
}

void DM_collectRF(DataPackageRF_t * package, int64_t time_us, Sensors_t * sensors, gps_t * gps, AHRS_t * ahrs, FlightState_t * flightstate, IGN_t * ign){
	package->id           = 0xAAAA;
	package->packet_no    = packet_counter++;
	package->packet_id    = 0x00AA;	//packet_id - 0x0001 -> first type of test frame
	package->timestamp_ms = (uint32_t)(time_us/1000);

	package->vbat_10  = 0;						// 1mV/LSB -> 100mV/LSB
	package->accX_100 = (int16_t)(sensors->LSM6DSO32.accX * 100.0f);
	package->accY_100 = (int16_t)(sensors->LSM6DSO32.accY * 100.0f);
	package->accZ_100 = (int16_t)(sensors->LSM6DSO32.accZ * 100.0f);

	package->gyroX_10 = (int16_t)(sensors->LSM6DSO32.gyroX * 100.0f);
	package->gyroY_10 = (int16_t)(sensors->LSM6DSO32.gyroY * 100.0f);
	package->gyroZ_10 = (int16_t)(sensors->LSM6DSO32.gyroZ * 100.0f);

	package->pressure = sensors->MS5607.press;

	package->lat      = (int32_t)(gps->latitude  * 10000000.0f);
	package->lon      = (int32_t)(gps->longitude * 10000000.0f);
	package->alti_gps = (int32_t)(gps->altitude);
	package->sats_fix     = ((gps->sats_in_use) & 0x3F) | (((uint8_t)(gps->fix)) << 6);
}
