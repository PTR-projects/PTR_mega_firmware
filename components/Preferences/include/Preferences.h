#pragma once


typedef struct{
	int mainAlt;
	int drougeAlt;

	float railLength;

	float maxAngle;

	float stagingDelay;
	float stagingMaxAngle;


}Preferences_driver_data_t;

esp_err_t Preferences_driver_init();
esp_err_t Preferences_driver_update(Preferences_driver_data_t config);
Preferences_driver_data_t Preferences_driver_get();
