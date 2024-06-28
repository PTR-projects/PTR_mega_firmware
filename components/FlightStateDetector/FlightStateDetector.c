#include <stdio.h>
#include <stdbool.h>
#include "esp_log.h"
#include "esp_err.h"
#include "AHRS_driver.h"
#include "Preferences.h"
#include "FlightStateDetector.h"

#define TIME_ELAPSED(start_ms, now_ms, wait_ms) (start_ms <= (now_ms - wait_ms))

//------ Private fun -----
static void FlightState_STARTUP				(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs);
static void FlightState_PREFLIGHT			(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs);
static void FlightState_ME_ACCELERATING		(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs);
static void FlightState_FREEFLIGHT			(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs);
static void FlightState_FREEFALL			(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs);
static void FlightState_DRAGCHUTE_FALL		(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs);
static void FlightState_MAINSHUTE_FALL		(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs);
static void FlightState_LANDING				(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs);

//----- Private var -----
static FlightState_t flightState_d;
static AHRS_t * 	 AHRS_ptr;
static FSD_settings_t FSD_settings_d;
static const char *TAG = "FSD";

static uint64_t stateChangeTime = 0;
static armingstatus_t armstatus_d = DISARMED;

void FSD_arming(){
	armstatus_d = ARMED;
}

void FSD_disarming(){
	armstatus_d = DISARMED;
}

armingstatus_t FSD_checkArmed(){
	return armstatus_d;
}

flightstate_t FSD_getState(){
	return flightState_d.state;
}

void FSD_forceState(flightstate_t new_state){
	flightState_d.state = new_state;
}

esp_err_t FSD_init(AHRS_t * ahrs){
	if(ahrs == NULL)
		return ESP_FAIL;

	Preferences_data_t pref;
	if(Preferences_get(&pref) == ESP_OK){
		FSD_settings_d.drouge_alt 		= (float)pref.drouge_alt_m;
		FSD_settings_d.main_alt 		= (float)pref.main_alt_m;
		FSD_settings_d.max_tilt 		= (float)pref.max_tilt_deg;
		FSD_settings_d.rail_height 		= (float)pref.rail_height_mm;
		FSD_settings_d.staging_delay_s 	= (float)pref.staging_delay_ms / 1000.0f;
		FSD_settings_d.staging_max_tilt = (float)pref.staging_max_tilt;
	}
	else {
		return ESP_FAIL;
	}

	AHRS_ptr = ahrs;

	flightState_d.state = FLIGHTSTATE_STARTUP;
	FSD_disarming();

	return ESP_OK;
}

esp_err_t FSD_detect(uint64_t time_ms){
	if(FSD_checkArmed() == DISARMED){
		flightState_d.state = FLIGHTSTATE_STARTUP;
		Sensors_UpdateReferencePressure();
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

	case FLIGHTSTATE_ME_ACCELERATING:
		FlightState_ME_ACCELERATING(time_ms, currentState, ahrs);
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

	case FLIGHTSTATE_MAINSHUTE_FALL:
		FlightState_MAINSHUTE_FALL(time_ms, currentState, ahrs);
		break;

	case FLIGHTSTATE_LANDING:
		FlightState_LANDING(time_ms, currentState, ahrs);
		break;

	case FLIGHTSTATE_SHUTDOWN:
		FSD_disarming();
		break;

	default:
		ESP_LOGE(TAG, "Wrong state! Disarming!");
		FSD_disarming();
	}

	return ESP_OK;
}

//----------- Private functions ----------------------------------
static void FlightState_STARTUP	(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs){
	//------ Komendy wykonywane tylko raz ------
	if(!(currentState->state_ready)) {
		currentState->state_ready = true;
		stateChangeTime = time_ms;
	}

	//------ Komendy wykonywane co pętlę ------
	Sensors_UpdateReferencePressure();
	Sensors_calibrateGyro(0.1f);


	//------ Warunki przejścia dalej ------
	if (TIME_ELAPSED(stateChangeTime, time_ms, 500)) {
		currentState->state = FLIGHTSTATE_PREFLIGHT;
		currentState->state_ready = false;
	}
}

static void FlightState_PREFLIGHT (uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs){
	//------ Komendy wykonywane tylko raz ------
	if(!(currentState->state_ready)) {
		currentState->state_ready = true;
		stateChangeTime = time_ms;
	}

	//------ Komendy wykonywane co pętlę ------
	Sensors_UpdateReferencePressure();
	Sensors_calibrateGyro(0.001f);

	//------ Warunki przejścia dalej ------
	if ((TIME_ELAPSED(stateChangeTime, time_ms, 100)) && (ahrs->acc_axis_lowpass >= (1.6f * 9.81f)) ) {
		currentState->state = FLIGHTSTATE_ME_ACCELERATING;
		currentState->state_ready = false;
	}
}

static void FlightState_ME_ACCELERATING	(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs){
	//------ Komendy wykonywane tylko raz ------
	if(!(currentState->state_ready)) {
		currentState->state_ready = true;
		stateChangeTime = time_ms;
		AHRS_setInFlight();
	}

	//------ Komendy wykonywane co pętlę ------


	//------ Warunki przejścia dalej ------
	if ((TIME_ELAPSED(stateChangeTime, time_ms, 200))  && (ahrs->acc_axis_lowpass < 0.0f) ) {
		currentState->state = FLIGHTSTATE_FREEFLIGHT;
		currentState->state_ready = false;
	}
}

static void FlightState_FREEFLIGHT (uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs){
	//------ Komendy wykonywane tylko raz ------
	if(!(currentState->state_ready)) {
		currentState->state_ready = true;
		stateChangeTime = time_ms;
	}

	//------ Komendy wykonywane co pętlę ------


	//------ Warunki przejścia dalej ------
	if ((TIME_ELAPSED(stateChangeTime, time_ms, 500))  && ((ahrs->max_altitude - ahrs->altitudeP) > 10.0f) ) {
		currentState->state = FLIGHTSTATE_FREEFALL;
		currentState->state_ready = false;
	}


	//------ Warunki wykrycia awarii ------
	if(0 /* && (SysManager_checkCriticalError() != ESP_OK) */) {
		currentState->state = FLIGHTSTATE_FREEFALL;
		currentState->state_ready = false;
	}
}

static void FlightState_FREEFALL (uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs){
	//------ Komendy wykonywane tylko raz ------
	if(!(currentState->state_ready)) {
		currentState->state_ready = true;
		stateChangeTime = time_ms;
	}

	//------ Komendy wykonywane co pętlę ------


	//------ Warunki przejścia dalej ------
	if ((TIME_ELAPSED(stateChangeTime, time_ms, 100)) && (1)) {
		currentState->state = FLIGHTSTATE_DRAGCHUTE_FALL;
		currentState->state_ready = false;
	}


	//------ Warunki wykrycia awarii ------
	if(0 /* && (SysManager_checkCriticalError() != ESP_OK) */) {
		currentState->state = FLIGHTSTATE_DRAGCHUTE_FALL;
		currentState->state_ready = false;
	}
}

static void FlightState_DRAGCHUTE_FALL (uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs){
	//------ Komendy wykonywane tylko raz ------
	if(!(currentState->state_ready)) {
		currentState->state_ready = true;
		stateChangeTime = time_ms;
	}

	//------ Komendy wykonywane co pętlę ------


	//------ Warunki przejścia dalej ------
	if ((TIME_ELAPSED(stateChangeTime, time_ms, 100) && (ahrs->altitudeP < FSD_settings_d.main_alt))
		|| (TIME_ELAPSED(stateChangeTime, time_ms, 2000) && (ahrs->ascent_rate < -60.0f)) ) {
		currentState->state = FLIGHTSTATE_MAINSHUTE_FALL;
		currentState->state_ready = false;
	}


	//------ Warunki wykrycia awarii ------
	if(0 /* && (SysManager_checkCriticalError() != ESP_OK) */) {
		currentState->state = FLIGHTSTATE_MAINSHUTE_FALL;
		currentState->state_ready = false;
	}
}

static void FlightState_MAINSHUTE_FALL (uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs){
	//------ Komendy wykonywane tylko raz ------
	if(!(currentState->state_ready)) {
		currentState->state_ready = true;
		stateChangeTime = time_ms;
	}

	//------ Komendy wykonywane co pętlę ------


	//------ Warunki przejścia dalej ------
	if (TIME_ELAPSED(stateChangeTime, time_ms, 10000) && (ahrs->velocityP > -0.5f)/* && (ahrs->pos.z < 200.0f) && (ahrs->vel.z > -1.0f)) */){
		currentState->state = FLIGHTSTATE_LANDING;
		currentState->state_ready = false;
	}


	//------ Warunki wykrycia awarii ------
	if(0 /* && (SysManager_checkCriticalError() != ESP_OK) */) {
		currentState->state = FLIGHTSTATE_LANDING;
		currentState->state_ready = false;
	}
}

static void FlightState_LANDING	(uint64_t time_ms, FlightState_t * currentState, AHRS_t * ahrs){
	//------ Komendy wykonywane tylko raz ------
	if(!(currentState->state_ready)) {
		currentState->state_ready = true;
		stateChangeTime = time_ms;
	}

	//------ Komendy wykonywane co pętlę ------

	//------ Warunki przejścia dalej ------
	if (TIME_ELAPSED(stateChangeTime, time_ms, 60000)){
		currentState->state = FLIGHTSTATE_SHUTDOWN;
		currentState->state_ready = false;
	}

	//------ Warunki wykrycia awarii ------
	if(0 /* && (SysManager_checkCriticalError() != ESP_OK) */) {
		//shut down safely
	}
}


