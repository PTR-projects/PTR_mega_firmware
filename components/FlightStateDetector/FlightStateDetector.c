#include <stdio.h>
#include <stdbool.h>
#include "esp_log.h"
#include "esp_err.h"
#include "AHRS_driver.h"
#include "Preferences.h"
#include "Mission.h"
#include "Effector_driver.h"
#include "FlightStateDetector.h"

//----- Our defines ----------
#define APO 			0
#define MAIN 			1
#define SECOND_STAGE	2
#define AUX 			3
 
//----- Private functions ----------
static void FlightState_STARTUP					(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs);
static void FlightState_PREFLIGHT				(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs);
static void FlightState_BOOST					(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs);
static void FlightState_SECOND_STAGE_DELAY		(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs);
static void FlightState_SECOND_STAGE_IGNITION	(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs);
static void FlightState_SECOND_STAGE_BOOST		(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs);
static void FlightState_FREEFLIGHT				(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs);
static void FlightState_FREEFALL				(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs);
static void FlightState_DRAGCHUTE_FALL			(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs);
static void FlightState_DRAGCHUTE_FAILURE		(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs);
static void FlightState_MAINSHUTE_FALL			(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs);
static void FlightState_RECOVERY_FAILURE		(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs);
static void FlightState_LANDING					(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs);

//----- Private variables ----------
static FlightState_t flightState_d;
static AHRS_t * 	 AHRS_ptr;
static const char *TAG = "FSD";

static uint64_t stateChangeTime = 0;

static void FSD_take(uint64_t time_ms, FlightState_t * currentState){
	float time_state_ms = (float)(time_ms - stateChangeTime);
	float time_launch_ms = 0.0f;
	if((flightState_d.ignition_time_ms != 0) && (time_ms >= flightState_d.ignition_time_ms)){
		time_launch_ms = (float)(time_ms - flightState_d.ignition_time_ms);
	}

	mission_fire_t fire;
	if(!Preferences_mission_poll((uint8_t)currentState->state, time_launch_ms, time_state_ms, &fire)){
		return;
	}

	for(uint8_t i = 0; i < fire.effector_count; i++){
		Effector_activate((effector_id_t)fire.effectors[i]);
	}
	if(fire.to_state == FLIGHTSTATE_BOOST){
		flightState_d.ignition_time_ms = time_ms;
	}
	currentState->state = (flightstate_t)fire.to_state;
	currentState->state_ready = false;
}
static armingstatus_t armstatus_d = DISARMED;

void FSD_arming(){
	armstatus_d = ARMED;

	Effector_armServos();
	Effector_armIgniters();
}

void FSD_disarming(){
	armstatus_d = DISARMED;
	Effector_disarmServos();
	Effector_disarmIgniters();
}

armingstatus_t IRAM_ATTR FSD_checkArmed(){
	return armstatus_d;
}

flightstate_t IRAM_ATTR FSD_getState(){
	return flightState_d.state;
}

void FSD_forceState(flightstate_t new_state){
	flightState_d.state = new_state;
}

esp_err_t FSD_init(AHRS_t * ahrs){
	if(ahrs == NULL){
		return ESP_FAIL;
	}

	Preferences_data_t pref;
	if(Preferences_get(&pref) != ESP_OK){
		return ESP_FAIL;
	}
	(void)pref;

	AHRS_ptr = ahrs;

	flightState_d.state = FLIGHTSTATE_STARTUP;
	FSD_disarming();
	

	return ESP_OK;
}

esp_err_t IRAM_ATTR FSD_detect(uint64_t time_ms){
	if(FSD_checkArmed() == DISARMED){
		flightState_d.state = FLIGHTSTATE_STARTUP;
		Sensors_UpdateReferencePressure();
		AHRS_resetMaxAltitude();
		Sensors_calibrateGyro(0.1f);
		return ESP_OK;
	}

	FlightState_t * currentState = &flightState_d;
	AHRS_t * 		ahrs 		 = AHRS_ptr;

	switch(flightState_d.state){
	case FLIGHTSTATE_STARTUP:
		FlightState_STARTUP(time_ms, currentState, ahrs);
		break;

	case FLIGHTSTATE_PREFLIGHT:
		FlightState_PREFLIGHT(time_ms, currentState, ahrs);
		break;

	case FLIGHTSTATE_BOOST:
		FlightState_BOOST(time_ms, currentState, ahrs);
		break;
	
	case FLIGHTSTATE_SECOND_STAGE_DELAY:
		FlightState_SECOND_STAGE_DELAY(time_ms, currentState, ahrs);
		break;

	case FLIGHTSTATE_SECOND_STAGE_IGNITION:
		FlightState_SECOND_STAGE_IGNITION(time_ms, currentState, ahrs);
		break;

	case FLIGHTSTATE_SECOND_STAGE_BOOST:
		FlightState_SECOND_STAGE_BOOST(time_ms, currentState, ahrs);
		break;
	
	case FLIGHTSTATE_FREEFLIGHT:
		FlightState_FREEFLIGHT(time_ms, currentState, ahrs);
		break;
	
	case FLIGHTSTATE_FREEFALL:
		FlightState_FREEFALL(time_ms, currentState, ahrs);
		break;

	case FLIGHTSTATE_DRAGCHUTE_FALL:
		FlightState_DRAGCHUTE_FALL(time_ms, currentState, ahrs);
		break;
	
	case FLIGHTSTATE_DRAGCHUTE_FAILURE:
		FlightState_DRAGCHUTE_FAILURE(time_ms, currentState, ahrs);
		break;

	case FLIGHTSTATE_MAINSHUTE_FALL:
		FlightState_MAINSHUTE_FALL(time_ms, currentState, ahrs);
		break;

	case FLIGHTSTATE_RECOVERY_FAILURE:
		FlightState_RECOVERY_FAILURE(time_ms, currentState, ahrs);
		break;

	case FLIGHTSTATE_LANDING:
		FlightState_LANDING(time_ms, currentState, ahrs);
		break;

	default:
		ESP_LOGE(TAG, "Wrong state! Disarming!");
		FSD_disarming();
	}

	return ESP_OK;
}


static void IRAM_ATTR FlightState_STARTUP(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs){
	(void)ahrs;
	//Executed only once
	if(!(currentState->state_ready)) {
		currentState->state_ready = true;
		stateChangeTime = time_ms;
	}

	//Executed every loop
	Sensors_UpdateReferencePressure();
	Sensors_calibrateGyro(0.1f);
	AHRS_resetMaxAltitude();


	FSD_take(time_ms, currentState);
}

static void IRAM_ATTR FlightState_PREFLIGHT(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs){
	(void)ahrs;
	//Executed only once
	if(!(currentState->state_ready)) {
		currentState->state_ready = true;
		stateChangeTime = time_ms;
	}

	//Executed every loop
	Sensors_UpdateReferencePressure();
	Sensors_calibrateGyro(0.001f);
	AHRS_resetMaxAltitude();

	FSD_take(time_ms, currentState);
}

static void IRAM_ATTR FlightState_BOOST(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs){
	(void)ahrs;
	//Executed only once
	if(!(currentState->state_ready)) {
		currentState->state_ready = true;
		stateChangeTime = time_ms;

		AHRS_setInFlight();
	}

	//Executed every loop


	FSD_take(time_ms, currentState);
}

static void IRAM_ATTR FlightState_SECOND_STAGE_DELAY(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs){
	(void)ahrs;
	//Executed only once
	if(!(currentState->state_ready)) {
		currentState->state_ready = true;
		stateChangeTime = time_ms;
	}

	//Executed every loop


	FSD_take(time_ms, currentState);
}

static void IRAM_ATTR FlightState_SECOND_STAGE_IGNITION(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs){
	(void)ahrs;
	//Executed only once
	if(!(currentState->state_ready)) {
		currentState->state_ready = true;
		stateChangeTime = time_ms;
	}

	//Executed every loop


	FSD_take(time_ms, currentState);
}

static void IRAM_ATTR FlightState_SECOND_STAGE_BOOST(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs){
	(void)ahrs;
	//Executed only once
	if(!(currentState->state_ready)) {
		currentState->state_ready = true;
		stateChangeTime = time_ms;
	}

	//Executed every loop


	FSD_take(time_ms, currentState);
}

static void IRAM_ATTR FlightState_FREEFLIGHT(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs){
	(void)ahrs;
	//Executed only once
	if(!(currentState->state_ready)) {
		currentState->state_ready = true;
		stateChangeTime = time_ms;
	}

	//Executed every loop


	FSD_take(time_ms, currentState);
}

static void IRAM_ATTR FlightState_FREEFALL(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs){
	(void)ahrs;
	//Executed only once
	if(!(currentState->state_ready)) {
		currentState->state_ready = true;
		stateChangeTime = time_ms;
	}

	//Executed every loop


	FSD_take(time_ms, currentState);
}

static void IRAM_ATTR FlightState_DRAGCHUTE_FALL(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs){
	(void)ahrs;
	//Executed only once
	if(!(currentState->state_ready)) {
		currentState->state_ready = true;
		stateChangeTime = time_ms;
	}

	//Executed every loop


	FSD_take(time_ms, currentState);
}

static void IRAM_ATTR FlightState_DRAGCHUTE_FAILURE(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs){
	(void)ahrs;
	//Executed only once
	if(!(currentState->state_ready)) {
		currentState->state_ready = true;
		stateChangeTime = time_ms;
	}

	//Executed every loop


	FSD_take(time_ms, currentState);
}

static void IRAM_ATTR FlightState_MAINSHUTE_FALL(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs){
	(void)ahrs;
	//Executed only once
	if(!(currentState->state_ready)) {
		currentState->state_ready = true;
		stateChangeTime = time_ms;
	}

	//Executed every loop


	FSD_take(time_ms, currentState);
}

static void IRAM_ATTR FlightState_RECOVERY_FAILURE(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs){
	(void)ahrs;
	//Executed only once
	if(!(currentState->state_ready)) {
		currentState->state_ready = true;
		stateChangeTime = time_ms;
	}

	//Executed every loop

	FSD_take(time_ms, currentState);
}

static void IRAM_ATTR FlightState_LANDING(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs){
	(void)ahrs;
	//Executed only once
	if(!(currentState->state_ready)) {
		currentState->state_ready = true;
		stateChangeTime = time_ms;
	}


	//Executed every loop

	FSD_take(time_ms, currentState);
}


