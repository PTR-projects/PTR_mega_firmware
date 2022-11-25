#include <stdio.h>
#include <math.h>
#include "esp_log.h"
#include "esp_err.h"
#include "Sensors.h"
#include "quaternion.h"
#include "common.h"
#include "KF_AltitudeAscent.h"
#include "AHRS_driver.h"

static void AHRS_CalcAltitudeP(float press);
static void AHRS_CalcVelocityPosition();
static void AHRS_CalcOrientation(Sensors_t * sensors);
static void AHRS_InitOrientation(orientation_t * orient);
static void AHRS_MahonyUpdate( float dt, 	float gx, float gy, float gz,
							uint8_t useAcc, float ax, float ay, float az,
							uint8_t useMag, float mx, float my, float mz,
							float dcmKpGain, orientation_t * orient);
static void AHRS_ComputeRotationMatrix(orientation_t * orient);
static void AHRS_UpdateEulerAngles(orientation_t * orient);
static void AHRS_TransformAccToENU();

static AHRS_t AHRS_d;
static float reference_pressure = 101300.0f;

// !!!! This library is NOT thread-safe !!!!

esp_err_t AHRS_init(int64_t time_us){
	AHRS_d.max_altitude = 0.0f;
	AHRS_d.prev_time_us = -1;

	AHRS_InitOrientation(&(AHRS_d.orientation));
	AHRS_kalmanAltitudeAscent_init(0.1f, 0.1f);

	return ESP_OK;
}

AHRS_t * AHRS_getData(){
	return &AHRS_d;
}

esp_err_t AHRS_compute(int64_t time_us, Sensors_t * sensors){
	// Calculate time diference and store new timestamp
	AHRS_d.dt = (time_us - AHRS_d.prev_time_us) / 1000000.0f;	//us to s
	AHRS_d.prev_time_us = time_us;

	AHRS_CalcAltitudeP(sensors->MS5607.press);
	AHRS_CalcOrientation(sensors);
	AHRS_TransformAccToENU();
	AHRS_CalcVelocityPosition();

	return ESP_OK;
}

void AHRS_UpdateReferencePressure(float press){
	AHRS_d.max_altitude = 0.0f;
	reference_pressure  = 0.05f*press + 0.95f*(reference_pressure);
}

//------------------ AHRS private functions -------------------
static void AHRS_CalcAltitudeP(float press){
	/** \desc Raw pressure data read from pressure sensor [Pa]. */
	float kalman_raw = press;
	/** \desc Pressure after current prediction [Pa]. */
	static float kalman_priori = 0.0f;
	/** \desc Pressure after current Kalman calculations [Pa]. */
	static float kalman_post;
	/** \desc Pressure after last Kalman calculations [Pa]. */
	static float kalman_postLast;
	/** \desc Speed of changes of calculated pressure [100*Pa/s]. */
	static float kalman_derivativePost = 0.0f;
	/** \desc Error Covariance, used to calculate gain in current Kalman prediction. */
	static float kalman_errorCovPriori;
	/** \desc Error Covariance, calculated in update, used to calculate errorCovPriori in next prediction. */
	static float kalman_errorCovPost = 1.0f;
	/** \desc Kalman update gain. */
	static float kalman_gain;
	/** \desc Kalman filter coefficient used when rocket ascending. */
	static float kalman_Q1 = 0.4f;
	/** \desc Kalman filter constant coefficient. */
	static float kalman_R = 207.0f;
	/** \desc Kalman filter is initialized? */
	static uint8_t kalman_initdone = 0;

	if(!kalman_initdone){
		kalman_post		= press;
		kalman_postLast = press;

		kalman_initdone = 1;
	}

	 if((press < 1000) && (press > 120000)){
		 //------- Prediction ------------
		 kalman_priori = kalman_post + kalman_derivativePost;		// Predict next data
		 kalman_errorCovPriori = kalman_errorCovPost + kalman_Q1;
		 kalman_gain = kalman_errorCovPriori / (kalman_errorCovPriori + kalman_R );	// Gain calculation

		 //------- Update ---------------
		 kalman_post = kalman_priori + kalman_gain * ( kalman_raw - kalman_priori);	// Update predicted data with actual data
		 kalman_derivativePost = kalman_derivativePost
				 	 	 	 	 + kalman_gain * ( kalman_post - kalman_postLast - kalman_derivativePost );
		 kalman_errorCovPost = (1.0f - kalman_gain) * kalman_errorCovPriori;	// Calculate new error covariance
		 kalman_postLast = kalman_post;	// Update temporary data for calculating difference between current and last sample
		//filters
	 }

	//ALTITUDE CALCULATIONS
	float alti_new =  (1.0-powf(kalman_post/reference_pressure, 0.190295f)) * 44330.0f;


	//wykrycie maksymalnego pułapu
	if((AHRS_d.max_altitude) < alti_new)
		AHRS_d.max_altitude = alti_new;

	AHRS_d.velocityP = 0.9f*AHRS_d.velocityP + 0.1f*(((alti_new) - AHRS_d.altitudeP) / AHRS_d.dt);
	AHRS_d.altitudeP = alti_new;

}

static void AHRS_CalcVelocityPosition(){
	AHRS_kalmanAltitudeAscent_step(AHRS_d.dt, AHRS_d.altitudeP, AHRS_d.acc_up,
			&(AHRS_d.altitude), &AHRS_d.ascent_rate);
}

static void AHRS_CalcOrientation(Sensors_t * sensors){
	bool useAcc = false;
	bool useMag = false;

	// TODO ------------------------------------------------------------------ Check flightstate to enable/disable acc or mag
	useMag = true;
	useAcc = true;

//	float dcmKpGain = imuCalcKpGain(AHRS_d.prev_time_us/1000, useAcc,	// TODO -------------- dynamic Kp gain
//			sensors->LSM6DSO32.gyroX, sensors->LSM6DSO32.gyroY, sensors->LSM6DSO32.gyroZ);
	float dcmKpGain = 2.5f;

	AHRS_MahonyUpdate(AHRS_d.dt,
						sensors->LSM6DSO32.gyroX, sensors->LSM6DSO32.gyroY, sensors->LSM6DSO32.gyroZ,
						useAcc, sensors->LSM6DSO32.accX, sensors->LSM6DSO32.accY, sensors->LSM6DSO32.accZ,
						useMag, sensors->MMC5983MA.magX, sensors->MMC5983MA.magY, sensors->MMC5983MA.magZ,
						dcmKpGain, &(AHRS_d.orientation));

	AHRS_UpdateEulerAngles(&(AHRS_d.orientation));
}

static void AHRS_InitOrientation(orientation_t * orient){
	orient->quaternions.w = 1.0f;
	orient->quaternions.x = 0.0f;
	orient->quaternions.y = 0.0f;
	orient->quaternions.z = 0.0f;

	orient->rMat[0][0] = 1.0f;
	orient->rMat[0][1] = 0.0f;
	orient->rMat[0][2] = 0.0f;

	orient->rMat[1][0] = 0.0f;
	orient->rMat[1][1] = 1.0f;
	orient->rMat[1][2] = 0.0f;

	orient->rMat[2][0] = 0.0f;
	orient->rMat[2][1] = 0.0f;
	orient->rMat[2][2] = 1.0f;

	orient->euler.tilt = 0.0f;
	orient->euler.dir   = 0.0f;
	orient->euler.rot  = 0.0f;
}

// https://github.com/betaflight/betaflight/blob/master/src/main/flight/imu.c
// https://github.com/iNavFlight/inav/blob/master/src/main/flight/imu.c
static void AHRS_MahonyUpdate( float dt, 	   float gx, float gy, float gz,
						 	   uint8_t useAcc, float ax, float ay, float az,
						 	   uint8_t useMag, float mx, float my, float mz,
							   float dcmKpGain, orientation_t * orient){
	// Integral error terms scaled by Ki
	static float integralFBx = 0.0f;
	static float integralFBy = 0.0f;
	static float integralFBz = 0.0f;

	// Errors
	float ex = 0, ey = 0, ez = 0;

	// Convert spin rate from deg/s to rad/s
	gx = DEGREES_TO_RADIANS(gx);
	gy = DEGREES_TO_RADIANS(gy);
	gz = DEGREES_TO_RADIANS(gz);

	// Calculate general spin rate (rad/s)
	float spin_rate = sqrtf(POW2(gx) + POW2(gy) + POW2(gz));

	// Rotate magnetic field from body to Earth and get horizontal component
	float recipMagNorm = POW2(mx) + POW2(my) + POW2(mz);
	if(useMag && (recipMagNorm > 0.01f)){
		recipMagNorm = 1/sqrtf(recipMagNorm);
		mx *= recipMagNorm;
		my *= recipMagNorm;
		mz *= recipMagNorm;

		// For magnetometer correction we make an assumption that magnetic field is perpendicular to gravity (ignore Z-component in EF).
		// This way magnetic field will only affect heading and wont mess roll/pitch angles

		// (hx; hy; 0) - measured mag field vector in EF (assuming Z-component is zero)
		// (bx; 0; 0) - reference mag field vector heading due North in EF (assuming Z-component is zero)
		const float hx = orient->rMat[0][0] * mx
						+ orient->rMat[0][1] * my
						+ orient->rMat[0][2] * mz;
		const float hy = orient->rMat[1][0] * mx
						+ orient->rMat[1][1] * my
						+ orient->rMat[1][2] * mz;
		const float bx = sqrtf(hx * hx + hy * hy);

		// magnetometer error is cross product between estimated magnetic north and measured magnetic north (calculated in EF)
		const float ez_ef = -(hy * bx);

		// Rotate mag error vector back to BF and accumulate
		ex += orient->rMat[2][0] * ez_ef;
		ey += orient->rMat[2][1] * ez_ef;
		ez += orient->rMat[2][2] * ez_ef;
	}

	// Use measured acceleration vector
	float recipAccNorm = POW2(ax) + POW2(ay) + POW2(az);
	if (useAcc && (recipAccNorm > 0.9f) && (recipAccNorm < 1.1f)) {
		// Normalise accelerometer measurement
		recipAccNorm = 1 / sqrtf(recipAccNorm);
		ax *= recipAccNorm;
		ay *= recipAccNorm;
		az *= recipAccNorm;

		// Error is sum of cross product between estimated direction and measured direction of gravity
		ex += (ay * orient->rMat[2][2] - az * orient->rMat[2][1]);
		ey += (az * orient->rMat[2][0] - ax * orient->rMat[2][2]);
		ez += (ax * orient->rMat[2][1] - ay * orient->rMat[2][0]);
	}

	// Compute and apply integral feedback if enabled
	//if (imuRuntimeConfig.dcm_ki > 0.0f) {					//<<--------------------------------------------zrobić zmienny Ki
	if(1){
		// Stop integrating if spinning beyond the certain limit
		if (spin_rate < DEGREES_TO_RADIANS(20)) {
			//float dcmKiGain = imuRuntimeConfig.dcm_ki;	//<<--------------------------------------------zrobić zmienny Ki
			float dcmKiGain = 30.0f / 10000.0f;
			integralFBx += dcmKiGain * ex * dt;    // integral error scaled by Ki
			integralFBy += dcmKiGain * ey * dt;
			integralFBz += dcmKiGain * ez * dt;
		}
	} else {
		integralFBx = 0.0f;    // prevent integral windup
		integralFBy = 0.0f;
		integralFBz = 0.0f;
	}

	// Apply proportional and integral feedback
	gx += dcmKpGain * ex + integralFBx;
	gy += dcmKpGain * ey + integralFBy;
	gz += dcmKpGain * ez + integralFBz;

	// Integrate rate of change of quaternion
	gx *= (0.5f * dt);
	gy *= (0.5f * dt);
	gz *= (0.5f * dt);

	quaternions_t buffer;
	buffer.w = orient->quaternions.w;
	buffer.x = orient->quaternions.x;
	buffer.y = orient->quaternions.y;
	buffer.z = orient->quaternions.z;

	orient->quaternions.w += (-buffer.x * gx - buffer.y * gy - buffer.z * gz);
	orient->quaternions.x += (+buffer.w * gx + buffer.y * gz - buffer.z * gy);
	orient->quaternions.y += (+buffer.w * gy - buffer.x * gz + buffer.z * gx);
	orient->quaternions.z += (+buffer.w * gz + buffer.x * gy - buffer.y * gx);

	// Normalise quaternion
	float recipNorm = 1 / sqrtf(POW2(orient->quaternions.w)
								+ POW2(orient->quaternions.x)
								+ POW2(orient->quaternions.y)
								+ POW2(orient->quaternions.z));
	orient->quaternions.w *= recipNorm;
	orient->quaternions.x *= recipNorm;
	orient->quaternions.y *= recipNorm;
	orient->quaternions.z *= recipNorm;

	// Pre-compute rotation matrix from quaternion
	AHRS_ComputeRotationMatrix(orient);
}

static void AHRS_ComputeRotationMatrix(orientation_t * orient){
	quaternionsProd_t qP;
	quaternionComputeProducts(&(orient->quaternions), &qP);

    orient->rMat[0][0] = 1.0f - 2.0f * qP.yy - 2.0f * qP.zz;
    orient->rMat[0][1] = 2.0f * (qP.xy + -qP.wz);
    orient->rMat[0][2] = 2.0f * (qP.xz - -qP.wy);

    orient->rMat[1][0] = 2.0f * (qP.xy - -qP.wz);
    orient->rMat[1][1] = 1.0f - 2.0f * qP.xx - 2.0f * qP.zz;
    orient->rMat[1][2] = 2.0f * (qP.yz + -qP.wx);

    orient->rMat[2][0] = 2.0f * (qP.xz + -qP.wy);
    orient->rMat[2][1] = 2.0f * (qP.yz - -qP.wx);
    orient->rMat[2][2] = 1.0f - 2.0f * qP.xx - 2.0f * qP.yy;
}

static void AHRS_UpdateEulerAngles(orientation_t * orient){
	quaternions_t q = orient->quaternions;

	float r11 =  2*(q.y*q.z - q.w*q.x);
	float r12 =  2*(q.x*q.z + q.w*q.y);
	float r21 =     q.w*q.w - q.x*q.x - q.y*q.y + q.z*q.z;
	float r31 =  2*(q.y*q.z + q.w*q.x);
	float r32 = -2*(q.x*q.z - q.w*q.y);

	float res_0 = atan2f(r11, r12);
	float res_1 = acosf(r21);
	float res_2 = atan2f(r31, r32);

	orient->euler.dir  = RADIANS_TO_DEGREES(res_0);		//minus dodany dla prawoskrętności E=0, N=90, W=+/-180, S=-90
	orient->euler.tilt = RADIANS_TO_DEGREES(res_1);
	orient->euler.rot  = RADIANS_TO_DEGREES(res_2);
}

static void AHRS_TransformAccToENU(){
	vectorf_t acc_enu;
	// From body frame to earth frame
	quaternionRotateVectorInv(&acc_enu, &(AHRS_d.acc_rf), &(AHRS_d.orientation.quaternions));

	// Store vertical acceleration
	AHRS_d.acc_up = acc_enu.z;
}


