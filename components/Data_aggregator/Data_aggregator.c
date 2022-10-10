#include <stdio.h>
#include "BOARD.h"
#include "Data_aggregator.h"

static uint16_t packet_counter = 0;

void Data_aggregate(DataPackage_t * package, int64_t time_us, Sensors_t * sensors, gps_t * gps, AHRS_t * ahrs, FlightState_t * flightstate, IGN_t * ign){

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

	//package->flightstate = (uint8_t)(flightstate->state);
}

#include <stdio.h>
#include "BOARD.h"
#include "Data_aggregator.h"

void Data_aggregateRF(DataPackageRF_t * package, int64_t time_us, Sensors_t * sensors, gps_t * gps, AHRS_t * ahrs, FlightState_t * flightstate, IGN_t * ign){
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
