#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "PTR_DataPacket.h"
#include "BOARD_cfg.h"
#include "DataManager.h"
#define DA_MAIN_QUEUE_SIZE 100

//--------------- Main Data Buffer ----------------
static DataPackage_t DataPackage_rb[DA_MAIN_QUEUE_SIZE] __attribute__((aligned(4)));

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
			ESP_LOGE(TAG, "Failed to add to Main RB!");
			return ESP_FAIL;
		}
	}

	ESP_LOGI(TAG, "RB init done");
	return ESP_OK;
}

uint16_t DM_checkWaitingElementsNumber(){
	return DA_MAIN_QUEUE_SIZE-uxQueueSpacesAvailable(queue_StorageUsed);
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
esp_err_t IRAM_ATTR DM_getFreePointerToMainRB(DataPackage_t ** ptr){
	if(xQueueReceive(queue_StorageFree, ptr, 0) != pdTRUE){
		//no free item in Free Queue get oldest from Used Queue
		if(xQueueReceive(queue_StorageUsed, ptr, 0) != pdTRUE)
				return ESP_FAIL;
	}

	return ESP_OK;
}

esp_err_t IRAM_ATTR DM_addToMainRB(DataPackage_t ** ptr){
	if(xQueueSend(queue_StorageUsed, ptr, 0) != pdTRUE)
		return ESP_FAIL;

	return ESP_OK;
}

void IRAM_ATTR DM_collectFlash(DataPackage_t * package, int64_t time_us, Sensors_t * sensors, gps_t * gps, AHRS_t * ahrs,
		flightstate_t flightstate, IGN_t * ign, Analog_meas_t * analog, servo_t * servo){

	assert(package != NULL);

	static Sensors_t sensors_d = {0};
	static gps_t gps_d = {0};
	static AHRS_t ahrs_d = {0};
	static IGN_t ign_d = {0};
	static Analog_meas_t analog_d = {0};
	static servo_t servo_d = {0};

	if(sensors == NULL)	sensors = &sensors_d;
	if(gps == NULL)		gps 	= &gps_d;
	if(ahrs == NULL)	ahrs 	= &ahrs_d;
	if(ign == NULL)		ign 	= &ign_d;
	if(analog == NULL)	analog 	= &analog_d;
	if(servo == NULL)	servo 	= &servo_d;

	package->sys_time = time_us/100;	// 0.1ms resolution

	package->sensors.accHX 		= sensors->LIS331.accX;
	package->sensors.accHY 		= sensors->LIS331.accY;
	package->sensors.accHZ 		= sensors->LIS331.accZ;

	package->sensors.accX 		= sensors->LSM6DSO32.accX;
	package->sensors.accY 		= sensors->LSM6DSO32.accY;
	package->sensors.accZ 		= sensors->LSM6DSO32.accZ;

	package->sensors.gyroX 		= sensors->LSM6DSO32.gyroX;
	package->sensors.gyroY 		= sensors->LSM6DSO32.gyroY;
	package->sensors.gyroZ 		= sensors->LSM6DSO32.gyroZ;

	package->sensors.magX 		= sensors->MMC5983MA.magX;
	package->sensors.magY 		= sensors->MMC5983MA.magY;
	package->sensors.magZ 		= sensors->MMC5983MA.magZ;

	package->sensors.pressure 	= sensors->MS5607.press;
	package->sensors.temp 		= (int8_t)sensors->MS5607.temp;

	package->sensors.latitude 	= gps->latitude;
	package->sensors.longitude 	= gps->longitude;
	package->sensors.altitude_gnss = gps->altitude;
	package->sensors.gnss_fix 	= ((gps->sats_in_use) & 0x3F) | (((uint8_t)(gps->fix)) << 6);

	package->ahrs.altitude_press 	 = ahrs->altitudeP;
	package->ahrs.altitude_kalman 	 = ahrs->altitude;
	package->ahrs.ascent_rate_kalman = ahrs->ascent_rate;

	package->ahrs.q0 = ahrs->orientation.quaternions.q0;
	package->ahrs.q1 = ahrs->orientation.quaternions.q1;
	package->ahrs.q2 = ahrs->orientation.quaternions.q2;
	package->ahrs.q3 = ahrs->orientation.quaternions.q3;
	package->ahrs.tilt = (uint8_t)AHRS_calcTilt(&(ahrs->orientation.euler));

#if IGN_NUM > 0
	package->ign.ign1_cont 		= analog->IGN_det[0];
	package->ign.ign1_state     = ign->igniter_state[0];
#if IGN_NUM > 1
	package->ign.ign2_cont 		= analog->IGN_det[1];
	package->ign.ign2_state     = ign->igniter_state[1];
#if IGN_NUM > 2
	package->ign.ign3_cont 		= analog->IGN_det[2];
	package->ign.ign3_state     = ign->igniter_state[2];
#if IGN_NUM > 3
	package->ign.ign4_cont 		= analog->IGN_det[3];
	package->ign.ign4_state     = ign->igniter_state[3];
#endif
#endif
#endif
#endif

	package->vbat_mV 			= (uint16_t)analog->vbat_mV;

	package->servo.servo_1 = (int8_t)servo->S1_pos;
	package->servo.servo_2 = (int8_t)servo->S2_pos;
	package->servo.servo_3 = (int8_t)servo->S3_pos;
	package->servo.servo_4 = (int8_t)servo->S4_pos;
	package->servo.servo_en = servo->servo_en;

	package->blank[0]			= 0;
	package->blank[1]			= 0;
	package->blank[2]			= 0;
	package->blank[3]			= 0;

	package->flightstate = (uint8_t)flightstate;
}

void IRAM_ATTR DM_collectRF(kppacket_t * package, int64_t time_us, Sensors_t * sensors, gps_t * gps, AHRS_t * ahrs, flightstate_t flightstate, IGN_t * ign, Analog_meas_t * analog){
	kppacket_payload_rocket_t payload = {0};

	assert(package != NULL);

	static Sensors_t sensors_d = {0};
	static gps_t gps_d = {0};
	static AHRS_t ahrs_d = {0};
	static IGN_t ign_d = {0};
	static Analog_meas_t analog_d = {0};
	//static servo_t servo_d = {0};

	if(sensors == NULL)	sensors = &sensors_d;
	if(gps == NULL)		gps 	= &gps_d;
	if(ahrs == NULL)	ahrs 	= &ahrs_d;
	if(ign == NULL)		ign 	= &ign_d;
	if(analog == NULL)	analog 	= &analog_d;
	//if(servo == NULL)	servo 	= &servo_d;

	payload.state       = (uint8_t)flightstate;
	payload.flags       = 0;
	payload.vbat_10     = (uint8_t)(analog->vbat_mV / 100.0f);
	payload.accX_100    = (int16_t)(sensors->LSM6DSO32.accX * 100.0f);
	payload.accY_100    = (int16_t)(sensors->LSM6DSO32.accY * 100.0f);
	payload.accZ_100    = (int16_t)(sensors->LSM6DSO32.accZ * 100.0f);
	payload.gyroX_10    = (int16_t)(sensors->LSM6DSO32.gyroX * 10.0f);
	payload.gyroY_10    = (int16_t)(sensors->LSM6DSO32.gyroY * 10.0f);
	payload.gyroZ_10    = (int16_t)(sensors->LSM6DSO32.gyroZ * 10.0f);
	payload.tilt_100    = (int16_t)(AHRS_calcTilt(&(ahrs->orientation.euler)) * 100.0f);
	payload.pressure    = sensors->MS5607.press;
	payload.velocity_10 = (int16_t)(ahrs->ascent_rate * 10.0f);
	payload.altitude    = (uint16_t)ahrs->altitude;
	payload.lat         = (int32_t)(gps->latitude  * 10000000.0f);
	payload.lon         = (int32_t)(gps->longitude * 10000000.0f);
	payload.alti_gps    = (int32_t)(gps->altitude);
	payload.sats_fix    = ((gps->sats_in_use) & 0x3F) | (((uint8_t)(gps->fix)) << 6);

	DataPacket_build_msg(package, PACKET_LEGACY_FULL, false,
	                     0, 0, packet_counter++,
	                     (uint32_t)(time_us / 1000),
	                     &payload, sizeof(payload));
}
 