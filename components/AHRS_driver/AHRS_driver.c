#include <stdio.h>
#include <math.h>
#include "esp_log.h"
#include "esp_err.h"

#include "AHRS_driver.h"

#include "Sensors.h"
#include "GNSS_driver.h"

#include "quaternion.h"
#include "common.h"
#include "KF_AltitudeAscent.h"


static const char *TAG = "AHRS";

#define GRAVITY 9.81f

static void AHRS_CalcAltitudeP(float press, float ref_press);
static void AHRS_CalcVelocityPosition();
static void AHRS_CalcApogeeEstimation();
static void AHRS_CalcOrientation(Sensors_t * sensors, bool useGyro);
static void AHRS_InitOrientation(orientation_t * orient);
static void AHRS_MahonyUpdate( float dt,
							uint8_t useGyro, float gx, float gy, float gz,
							uint8_t useAcc,  float ax, float ay, float az,
							uint8_t useMag,  float mx, float my, float mz,
							float dcmKpGain, orientation_t * orient);
static void AHRS_ComputeRotationMatrix(orientation_t * orient);
static void AHRS_UpdateEulerAngles(orientation_t * orient);
static void AHRS_TransformAccToENU();

static AHRS_t AHRS_d;
static uint8_t orientation_useAcc = 1;
static uint8_t orientation_useMag = 0;
static bool flag_in_flight = false;


esp_err_t AHRS_init(int64_t time_us){
	AHRS_d.max_altitude        =  0.0f;
	AHRS_d.apogee_altitude_est =  0.0f;
	AHRS_d.time_to_apogee_est  =  0.0f;
	AHRS_d.prev_time_us        = (uint64_t)time_us;

	AHRS_InitOrientation(&(AHRS_d.orientation));
	AHRS_kalmanAltitudeAscent_init(0.1f, 0.1f);
	

	return ESP_OK;
}

AHRS_t * IRAM_ATTR AHRS_getData(){
	return &AHRS_d;
}

void AHRS_resetMaxAltitude(){
	AHRS_d.max_altitude = AHRS_d.altitudeP;
}

esp_err_t IRAM_ATTR AHRS_compute(int64_t time_us, Sensors_t * sensors, gps_t gps){
	// Calculate time diference and store new timestamp
	AHRS_d.dt = (time_us - AHRS_d.prev_time_us) / 1000000.0f;	//us to s
	AHRS_d.prev_time_us = time_us;

	//Store raw accelerations in AHRS_d <<---- to be changed with more complex function (sensors fusion)
	AHRS_d.acc_rf.x = GRAVITY * sensors->LSM6DSO32.accX;
	AHRS_d.acc_rf.y = GRAVITY * sensors->LSM6DSO32.accY;
	AHRS_d.acc_rf.z = GRAVITY * sensors->LSM6DSO32.accZ;

	AHRS_CalcAltitudeP(sensors->MS5607.press, sensors->ref_press);
	AHRS_d.acc_axis_lowpass = 0.05f*AHRS_d.acc_rf.x + 0.95f*AHRS_d.acc_axis_lowpass;

	if(flag_in_flight == false){
		AHRS_CalcOrientation(sensors, false);
		AHRS_TransformAccToENU();
	}
	else {
		AHRS_CalcOrientation(sensors, true);
		AHRS_TransformAccToENU();
		AHRS_CalcVelocityPosition();
		AHRS_CalcApogeeEstimation();
	}

	// Zero-Velocity Update (ZUPT) - PRE-LAUNCH ONLY. While the board sits on the pad,
	// force horizontal ENU velocity to 0: kills inertial drift and the start-up transient.
	// Gated to !flag_in_flight so that once launched the velocity integrates freely (needed for
	// guidance, and so a stationary board on the bench after launch is NOT zeroed). In real flight
	// this gate is moot (boost has high |a|, coast has |a|~0, so the rest test never fires anyway).
	float zupt_gyro = sqrtf(POW2(sensors->LSM6DSO32.gyroX) + POW2(sensors->LSM6DSO32.gyroY) + POW2(sensors->LSM6DSO32.gyroZ)); // deg/s
	float zupt_acc  = sqrtf(POW2(sensors->LSM6DSO32.accX)  + POW2(sensors->LSM6DSO32.accY)  + POW2(sensors->LSM6DSO32.accZ));  // g
	if((flag_in_flight == false) && (zupt_gyro < 5.0f) && (fabsf(zupt_acc - 1.0f) < 0.05f)){
		AHRS_d.vel_enu.x = 0.0f;
		AHRS_d.vel_enu.y = 0.0f;
	}


	/*static uint32_t pos_log_cnt = 0;
	if(++pos_log_cnt >= 20){
		pos_log_cnt = 0;
		ESP_LOGI(TAG, "eul[deg] r=%.1f p=%.1f y=%.1f | acc_enu[m/s2] x=%.2f y=%.2f z=%.2f | vel[m/s] x=%.2f y=%.2f z=%.2f | pos[m] x=%.1f y=%.1f z=%.1f | acc_norm %.2f",
				 AHRS_d.orientation.euler.roll, AHRS_d.orientation.euler.pitch, AHRS_d.orientation.euler.yaw,
				 AHRS_d.acc_enu.x, AHRS_d.acc_enu.y, AHRS_d.acc_enu.z,
				 AHRS_d.vel_enu.x, AHRS_d.vel_enu.y, AHRS_d.vel_enu.z,
				 AHRS_d.pos_enu.x, AHRS_d.pos_enu.y, AHRS_d.pos_enu.z,
				sqrtf(AHRS_d.acc_enu.x*AHRS_d.acc_enu.x + AHRS_d.acc_enu.y*AHRS_d.acc_enu.y + AHRS_d.acc_enu.z*AHRS_d.acc_enu.z));
	}*/

	return ESP_OK;
}

void AHRS_orientationSettings(uint8_t enableAcc, uint8_t enableMag){
	orientation_useAcc = enableAcc;
	orientation_useMag = enableMag;
}

void AHRS_resetVelocityPosition(){
	AHRS_d.vel_enu.x = 0.0f;
	AHRS_d.vel_enu.y = 0.0f;
	AHRS_d.vel_enu.z = 0.0f;
	AHRS_d.pos_enu.x = 0.0f;
	AHRS_d.pos_enu.y = 0.0f;
	AHRS_d.pos_enu.z = 0.0f;
}

void AHRS_setInFlight(){
	flag_in_flight = true;
	orientation_useAcc = 0;

	// Start earth-frame velocity/position integration from zero at launch (rocket ~stationary on rail).
	AHRS_resetVelocityPosition();
}

//------------------ AHRS private functions -------------------
// Arecorder Kalman for pressure and altitude
static void IRAM_ATTR AHRS_CalcAltitudeP(float press, float ref_press){
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
	static float kalman_w = 0.4f;
	/** \desc Kalman filter constant coefficient. */
	static float kalman_R = 207.0f;
	/** \desc Kalman filter is initialized? */
	static uint8_t kalman_initdone = 0;

	if(!kalman_initdone){
		kalman_post		= press;
		kalman_postLast = press;

		kalman_initdone = 1;
	}

	 if((press > 1000) && (press < 120000)){
		 //------- Prediction ------------
		 kalman_priori = kalman_post + kalman_derivativePost;		// Predict next data
		 kalman_errorCovPriori = kalman_errorCovPost + kalman_w;
		 kalman_gain = kalman_errorCovPriori / (kalman_errorCovPriori + kalman_R );	// Gain calculation

		 //------- Update ---------------
		 kalman_post = kalman_priori + kalman_gain * ( kalman_raw - kalman_priori);	// Update predicted data with actual data
		 kalman_derivativePost = kalman_derivativePost
				 	 	 	 	 + kalman_gain * ( kalman_post - kalman_postLast - kalman_derivativePost );
		 kalman_errorCovPost = (1.0f - kalman_gain) * kalman_errorCovPriori;	// Calculate new error covariance
		 kalman_postLast = kalman_post;	// Update temporary data for calculating difference between current and last sample
		//filters
	 }

	
	float alti_new =  (1.0-powf(kalman_post/ref_press, 0.190295f)) * 44330.0f;
	AHRS_d.velocityP = 0.95f*AHRS_d.velocityP + 0.05f*(((alti_new) - AHRS_d.altitudeP) / AHRS_d.dt);
	AHRS_d.altitudeP = alti_new;
	
	if((AHRS_d.max_altitude) < AHRS_d.altitudeP){
		AHRS_d.max_altitude = AHRS_d.altitudeP;
	}

	//ESP_LOGI(TAG, "Altitude: %f, Max: %f",AHRS_d.altitudeP, AHRS_d.max_altitude);
}

static void IRAM_ATTR AHRS_CalcVelocityPosition(){
	AHRS_kalmanAltitudeAscent_step(AHRS_d.dt, AHRS_d.altitudeP, AHRS_d.acc_up, &(AHRS_d.altitude), &AHRS_d.ascent_rate);

	// Vertical velocity/position from the baro-fused altitude Kalman (less drift than integration).
	// Horizontal velocity/position are integrated in AHRS_TransformAccToENU() so they run pre-launch too.
	AHRS_d.vel_enu.z = AHRS_d.ascent_rate;
	AHRS_d.pos_enu.z = AHRS_d.altitude;
}

static void IRAM_ATTR AHRS_CalcApogeeEstimation(){
	if(AHRS_d.ascent_rate > 0.0f){
		AHRS_d.time_to_apogee_est  = AHRS_d.ascent_rate / GRAVITY;
		AHRS_d.apogee_altitude_est = AHRS_d.altitude + (AHRS_d.ascent_rate * AHRS_d.ascent_rate) / (2.0f * GRAVITY);
	}
}

static void IRAM_ATTR AHRS_CalcOrientation(Sensors_t * sensors, bool useGyro){
	bool useMag = orientation_useMag;
	bool useAcc = orientation_useAcc;
	float dcmKpGain = 2.5f;

	AHRS_MahonyUpdate(AHRS_d.dt,
						useGyro, sensors->LSM6DSO32.gyroX, sensors->LSM6DSO32.gyroY, sensors->LSM6DSO32.gyroZ,
						useAcc,  sensors->LSM6DSO32.accX,  sensors->LSM6DSO32.accY,  sensors->LSM6DSO32.accZ,
						useMag,  sensors->MMC5983MA.magX,  sensors->MMC5983MA.magY,  sensors->MMC5983MA.magZ,
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

	orient->euler.pitch = 0.0f;
	orient->euler.yaw  = 0.0f;
	orient->euler.roll  = 0.0f;
}

// https://github.com/betaflight/betaflight/blob/master/src/main/flight/imu.c
// https://github.com/iNavFlight/inav/blob/master/src/main/flight/imu.c
static void IRAM_ATTR AHRS_MahonyUpdate( float dt,
		 	 	 	 	 	   uint8_t useGyro, float gx, float gy, float gz,
						 	   uint8_t useAcc,  float ax, float ay, float az,
						 	   uint8_t useMag,  float mx, float my, float mz,
							   float dcmKpGain,  orientation_t * orient){
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

	
	// If gyro disabled -> zero gyro meas
	if(!useGyro){
		gx = 0.0f;
		gy = 0.0f;
		gz = 0.0f;
	}

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
		const float hx  = orient->rMat[0][0] * mx
						+ orient->rMat[0][1] * my
						+ orient->rMat[0][2] * mz;
		const float hy  = orient->rMat[1][0] * mx
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
	float accNormSq = POW2(ax) + POW2(ay) + POW2(az);
	if (useAcc && (accNormSq > 0.9f) && (accNormSq < 1.1f)) {
		// Normalise accelerometer measurementS
		float recipAccNorm = 1 / sqrtf(accNormSq);
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
	float recipNorm = 1.0f / sqrtf(POW2(orient->quaternions.w)
								+ POW2(orient->quaternions.x)
								+ POW2(orient->quaternions.y)
								+ POW2(orient->quaternions.z));
	orient->quaternions.w *= recipNorm;
	orient->quaternions.x *= recipNorm;
	orient->quaternions.y *= recipNorm;
	orient->quaternions.z *= recipNorm;

	AHRS_ComputeRotationMatrix(orient);
}

static void IRAM_ATTR AHRS_ComputeRotationMatrix(orientation_t * orient){
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

static void IRAM_ATTR AHRS_UpdateEulerAngles(orientation_t * orient){
	
	float sinp = -1.0f * orient->rMat[2][0];
	if(sinp >  1.0f) sinp =  1.0f;
	if(sinp < -1.0f) sinp = -1.0f;

	orient->euler.roll  = RADIANS_TO_DEGREES(atan2f(orient->rMat[2][1],orient->rMat[2][2]));
	orient->euler.pitch = RADIANS_TO_DEGREES(asinf(sinp));
	orient->euler.yaw   = RADIANS_TO_DEGREES(atan2f(orient->rMat[1][0],orient->rMat[0][0]));

	//ESP_LOGI(TAG, "%f, %f, %f, %f", AHRS_d.orientation.quaternions.w, AHRS_d.orientation.quaternions.x, AHRS_d.orientation.quaternions.y, AHRS_d.orientation.quaternions.z);
	//ESP_LOGI(TAG, "%f, %f, %f", AHRS_d.orientation.euler.roll, AHRS_d.orientation.euler.pitch, AHRS_d.orientation.euler.yaw);

}

static void IRAM_ATTR AHRS_TransformAccToENU(){
	vectorf_t acc_enu;

	vectorf_t acc_rf;

	acc_rf.x =  AHRS_d.acc_rf.x;
	acc_rf.y = 	AHRS_d.acc_rf.y;
	acc_rf.z =  AHRS_d.acc_rf.z;
	quaternionRotateVectorInv(&acc_enu, &acc_rf, &(AHRS_d.orientation.quaternions));

	AHRS_d.acc_up = acc_enu.z - GRAVITY;
	AHRS_d.acc_enu = acc_enu;

	// Integrate horizontal ENU acceleration -> velocity -> position.
	// Runs every cycle (including pre-launch) so the guidance can be exercised on the bench.
	// Zeroed at launch in AHRS_setInFlight() and on demand via AHRS_resetVelocityPosition().
	// Pure integration drifts with accel bias/tilt error - fine for a short bench move-test,
	// bounded in flight by the launch reset + short flight time.
	AHRS_d.vel_enu.x += acc_enu.x * AHRS_d.dt;
	AHRS_d.vel_enu.y += acc_enu.y * AHRS_d.dt;
	AHRS_d.pos_enu.x += AHRS_d.vel_enu.x * AHRS_d.dt;
	AHRS_d.pos_enu.y += AHRS_d.vel_enu.y * AHRS_d.dt;


	/*static uint32_t pos_log_cnt = 0;
	if(++pos_log_cnt >= 5){
		pos_log_cnt = 0;
		//ESP_LOGI(TAG, "%f", AHRS_d.acc_up);
		//ESP_LOGI(TAG, "%f, %f, %f", acc_enu.x, acc_enu.y,acc_enu.z);
		//ESP_LOGI(TAG, "%f, %f, %f", acc_rf.x, acc_rf.y,acc_rf.z);
	}*/
}