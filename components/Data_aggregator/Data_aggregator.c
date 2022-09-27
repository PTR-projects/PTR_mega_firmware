#include <stdio.h>
#include "BOARD.h"
#include "Data_aggregator.h"

void Data_aggregate(DataPackage_t * package, int64_t time_us, Sensors_t * sensors, AHRS_t * ahrs, FlightState_t * flightstate, IGN_t * ign){

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

	package->sensors.latitude = 0;
	package->sensors.longitude = 0;
	package->sensors.altitude_gnss = 0;
	package->sensors.gnss_fix = 0;

	package->flightstate = (uint8_t)(flightstate->state);
}
