#include "include/Preferences.h"

#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_system.h"
#include <rom/crc.h>

#include <stdlib.h>
#include <errno.h>

#include "esp_spiffs.h"
#include "cJSON.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "BOARD_cfg.h"

static bool json_check_cfg(cJSON * json);
static int preferences_tracker_khz(int khz);
static uint64_t preferences_parse_lora_key(const cJSON *item);
static void preferences_add_lora_key(cJSON *json, uint64_t key);
static void preferences_store_pass(const char *pass);
static size_t preferences_pass_len(const char *pass);
static uint64_t preferences_key_from_pass(const char *pass);

static const char *TAG = "Preferences_driver";
const char* preferences_path = "/www/preferences.txt";

static bool pref_init_done = false;
static Preferences_data_t Preferences_data_d;
static Preferences_data_t Preferences_default;
static volatile char wifi_pass[64] = { CONFIG_ESP_WIFI_PASSWORD };	// max password length for WPA2 is 63 characters - we need 1 more for '/0'
static char lora_pass_buf[64];

esp_err_t Preferences_init(){
	ESP_LOGI(TAG, "Preferences init");

	char buf[1024] = {0};

	//Deafult KPPTR configuration
	Preferences_default.main_alt_m 			= 200;
	Preferences_default.drouge_alt_m 		= 0;
	Preferences_default.max_tilt_deg 		= 45;
	Preferences_default.staging_delay_ms 	= 0;
	Preferences_default.rail_height_mm 		= 2000;
	Preferences_default.auto_arming 		= true;
	Preferences_default.auto_arming_time_s 	= 60;
	Preferences_default.lora_freq_khz 		= 433125;
	Preferences_default.lora_tracker_freq_khz = 434250;
	Preferences_default.wifi_pass = (char*) wifi_pass;
	preferences_store_pass("");
	Preferences_default.lora_pass			= lora_pass_buf;
	Preferences_default.lora_key 			= PREFERENCES_LORA_KEY_FACTORY;

	// Check if configuration file exists
	struct stat st;
	int8_t FileStatus = stat(preferences_path, &st);

	if(FileStatus == -1){
		ESP_LOGW(TAG, "Config file not present, creating file");
		Preferences_update(Preferences_default, false);
	}
	else{
		ESP_LOGI(TAG, "Config file present");
	}

	// Load configuration file and parse its content
	FILE* f = fopen(preferences_path, "r");
	if(f == NULL){
		ESP_LOGE(TAG, "Failed to open file for reading");
		Preferences_data_d = Preferences_default;
		pref_init_done = true;
		return ESP_ERR_NOT_FOUND;
	}

	fseek(f, 0L, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0L, SEEK_SET);
	ESP_LOGI(TAG, "Configuration file size: %d", size);

	if(size == 0 || size >= sizeof(buf)){
		ESP_LOGE(TAG, "Config file size invalid");
		fclose(f);
		Preferences_update(Preferences_default, false);
		Preferences_data_d = Preferences_default;
		pref_init_done = true;
		return ESP_ERR_INVALID_SIZE;
	}

	fread(buf, size , 1, f);	// Read file (config + 2 bytes of CRC16)
	fclose(f);

	// Check file checksum using ESP32 ROM CRC library
	uint16_t romCRC = crc16_le(0, 2+(const uint8_t*)buf, strlen(buf+2));
	ESP_LOGI(TAG, "Config file: %s", buf+2);

	if(romCRC != *((uint16_t*)buf)){
		// File corrupted
		ESP_LOGE(TAG, "CRC ERROR!");
		Preferences_update(Preferences_default, false);
		Preferences_data_d = Preferences_default;
		pref_init_done = true;
		return ESP_ERR_INVALID_CRC;
	}

	ESP_LOGI(TAG, "CRC OK!");
	cJSON *json = cJSON_Parse(buf+2);

	if(!json_check_cfg(json)) {
		ESP_LOGE(TAG, "File format not up-to-date! Changing...");
		Preferences_update(Preferences_default, false);
		Preferences_data_d = Preferences_default;
		pref_init_done = true;
		return ESP_FAIL;
	}

	ESP_LOGI(TAG, "JSON OK");
	Preferences_data_d.main_alt_m 			= cJSON_GetObjectItem(json, "main_alt")->valueint;
	Preferences_data_d.drouge_alt_m 		= cJSON_GetObjectItem(json, "drouge_alt")->valueint;
	Preferences_data_d.max_tilt_deg 		= cJSON_GetObjectItem(json, "max_tilt")->valueint;
	Preferences_data_d.staging_delay_ms 	= cJSON_GetObjectItem(json, "staging_delay")->valueint;
	Preferences_data_d.staging_max_tilt 	= cJSON_GetObjectItem(json, "staging_max_tilt")->valueint;
	Preferences_data_d.rail_height_mm 		= cJSON_GetObjectItem(json, "rail_height")->valueint;
	Preferences_data_d.auto_arming 			= cJSON_GetObjectItem(json, "auto_arming")->valueint;
	Preferences_data_d.auto_arming_time_s 	= cJSON_GetObjectItem(json, "auto_arming_time_s")->valueint;
	Preferences_data_d.lora_freq_khz 		= cJSON_GetObjectItem(json, "lora_freq")->valueint;
	{
		cJSON *tracker = cJSON_GetObjectItem(json, "lora_tracker_freq");
		Preferences_data_d.lora_tracker_freq_khz = tracker
			? preferences_tracker_khz(tracker->valueint)
			: 434250;
	}
	Preferences_data_d.lora_mode 			= cJSON_GetObjectItem(json, "lora_mode")->valueint;
	{
		cJSON *pass = cJSON_GetObjectItem(json, "lora_pass");
		if(cJSON_IsString(pass) && preferences_pass_len(pass->valuestring) >= PREFERENCES_LORA_PASS_MIN){
			preferences_store_pass(pass->valuestring);
			Preferences_data_d.lora_pass = lora_pass_buf;
			Preferences_data_d.lora_key = preferences_key_from_pass(lora_pass_buf);
		} else if(cJSON_IsString(pass)){
			preferences_store_pass("");
			Preferences_data_d.lora_pass = lora_pass_buf;
			Preferences_data_d.lora_key = PREFERENCES_LORA_KEY_FACTORY;
		} else {
			preferences_store_pass("");
			Preferences_data_d.lora_pass = lora_pass_buf;
			Preferences_data_d.lora_key = preferences_parse_lora_key(cJSON_GetObjectItem(json, "lora_key"));
		}
	}

	Preferences_data_d.wifi_pass 			= (char*)wifi_pass;
	strncpy(Preferences_data_d.wifi_pass,
	        cJSON_GetObjectItem(json, "wifi_pass")->valuestring,
	        sizeof(wifi_pass) - 1);
	Preferences_data_d.wifi_pass[sizeof(wifi_pass) - 1] = '\0';

	cJSON_Delete(json);
	pref_init_done = true;
	
	return ESP_OK;
}

esp_err_t Preferences_get(Preferences_data_t * ptr){
	if(pref_init_done){
		*ptr = Preferences_data_d;
		return ESP_OK;
	}

	return ESP_ERR_NOT_FINISHED;
}


/*!
 * @brief Update configuration, firstly update in RAM then copy to file to prevent reading errors
 * @param config
 * Struct with new config data
 * @return `ESP_OK` if
 * @return `ESP_ERR_NOT_FOUND`
 */
esp_err_t Preferences_update(Preferences_data_t config, bool req_restart){
	ESP_LOGI(TAG, "Config update");
	char conf_file[1024] = {0};
	cJSON *json = cJSON_CreateObject();
	
	//Update current config stored in RAM. A password replaces any stored key.
	preferences_store_pass(config.lora_pass);
	if(lora_pass_buf[0] != '\0' && strlen(lora_pass_buf) < PREFERENCES_LORA_PASS_MIN)
		preferences_store_pass("");
	config.lora_pass = lora_pass_buf;
	if(strlen(lora_pass_buf) >= PREFERENCES_LORA_PASS_MIN)
		config.lora_key = preferences_key_from_pass(lora_pass_buf);
	else if(config.lora_key == 0)
		config.lora_key = PREFERENCES_LORA_KEY_FACTORY;
	Preferences_data_d = config;
	cJSON_AddStringToObject(json, "wifi_pass", 			Preferences_data_d.wifi_pass);
	cJSON_AddNumberToObject(json, "main_alt", 			Preferences_data_d.main_alt_m);
	cJSON_AddNumberToObject(json, "drouge_alt",			Preferences_data_d.drouge_alt_m);
	cJSON_AddNumberToObject(json, "rail_height", 		Preferences_data_d.rail_height_mm);
	cJSON_AddNumberToObject(json, "max_tilt", 			Preferences_data_d.max_tilt_deg);
	cJSON_AddNumberToObject(json, "staging_delay", 		Preferences_data_d.staging_delay_ms);
	cJSON_AddNumberToObject(json, "staging_max_tilt", 	Preferences_data_d.staging_max_tilt);
	cJSON_AddNumberToObject(json, "auto_arming_time_s", Preferences_data_d.auto_arming_time_s);
	cJSON_AddNumberToObject(json, "auto_arming", 		Preferences_data_d.auto_arming);
	cJSON_AddNumberToObject(json, "lora_freq", 			Preferences_data_d.lora_freq_khz);
	cJSON_AddNumberToObject(json, "lora_tracker_freq", 	Preferences_data_d.lora_tracker_freq_khz);
	cJSON_AddNumberToObject(json, "lora_mode", 			Preferences_data_d.lora_mode);
	cJSON_AddStringToObject(json, "lora_pass", 			lora_pass_buf);
	preferences_add_lora_key(json, Preferences_data_d.lora_key);
	
	if(cJSON_PrintPreallocated(json, conf_file+2, sizeof(conf_file), 0) == 0){
		ESP_LOGE(TAG, "Cannot create JSON string");
		return ESP_FAIL;
	}
	cJSON_Delete(json); 

	// Add CRC16 at the end of the string
	uint32_t romCRC = crc16_le(0, 2+(const uint8_t*)conf_file, strlen(conf_file+2));
	*((uint16_t*)conf_file) = romCRC;

	ESP_LOGI(TAG, "New Config file: %s", conf_file+2);
	
	//Update config stored on FLASH chip
	remove(preferences_path);
	FILE* f = fopen(preferences_path, "w");
	if(f == NULL){
		ESP_LOGE(TAG, "Failed to open file for writing");
		return ESP_ERR_NOT_FOUND;
	}

	fwrite(conf_file, 1, 2+strlen(conf_file+2), f);
	fwrite("\0", 1, 1, f);
	fclose(f);
	ESP_LOGI(TAG, "Updated config successfully");
	
	if(req_restart){
		esp_restart();
	}

	return ESP_FAIL; //If it returns it means that esp failed to restart
}

esp_err_t Preferences_restore_dafaults(){
	preferences_store_pass("");
	Preferences_default.lora_pass = lora_pass_buf;
	Preferences_default.lora_key = PREFERENCES_LORA_KEY_FACTORY;
	return Preferences_update(Preferences_default, false);
}

esp_err_t Prefences_update_web(char *buf){
	if(buf == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	Preferences_data_t temp = Preferences_data_d;
	uint16_t crc_from_json = atoi(buf);

	buf = strchr(buf, '{');
	if(buf == NULL) {
		return ESP_FAIL;
	}

	cJSON *json = cJSON_Parse(buf);
	ESP_LOGI(TAG, "New from web:\n%s", buf);

	if(!json_check_cfg(json)) {
		ESP_LOGE(TAG, "Cannot read json!");
		return ESP_FAIL;
	}
	
	uint16_t crc16_received = crc_from_json;
	uint16_t romCRC = ~crc16_be((uint16_t)~0xffff, (const uint8_t*)buf, strlen(buf));

	if(crc16_received != romCRC) {
		ESP_LOGE(TAG, "CRC error");
		return ESP_FAIL;
	}

	ESP_LOGI(TAG, "JSON CRC OK: 0x%x", crc16_received);
	temp.wifi_pass 			= cJSON_GetObjectItem(json, "wifi_pass")->valuestring;
	temp.main_alt_m 		= cJSON_GetObjectItem(json, "main_alt")->valueint;
	temp.drouge_alt_m 		= cJSON_GetObjectItem(json, "drouge_alt")->valueint;
	temp.rail_height_mm 	= cJSON_GetObjectItem(json, "rail_height")->valueint;
	temp.max_tilt_deg 		= cJSON_GetObjectItem(json, "max_tilt")->valueint;
	temp.staging_delay_ms 	= cJSON_GetObjectItem(json, "staging_delay")->valueint;
	temp.staging_max_tilt 	= cJSON_GetObjectItem(json, "staging_max_tilt")->valueint;
	temp.auto_arming_time_s = cJSON_GetObjectItem(json, "auto_arming_time_s")->valueint;
	temp.auto_arming 		= cJSON_GetObjectItem(json, "auto_arming")->valueint;
	temp.key 				= cJSON_GetObjectItem(json, "key")->valueint;
	temp.lora_freq_khz 		= cJSON_GetObjectItem(json, "lora_freq")->valueint;
	{
		cJSON *tracker = cJSON_GetObjectItem(json, "lora_tracker_freq");
		int tracker_khz = tracker ? tracker->valueint : Preferences_data_d.lora_tracker_freq_khz;
		temp.lora_tracker_freq_khz = preferences_tracker_khz(tracker_khz);
	}
	temp.lora_mode 			= cJSON_GetObjectItem(json, "lora_mode")->valueint;
	{
		cJSON *pass = cJSON_GetObjectItem(json, "lora_pass");
		if(cJSON_IsString(pass)){
			if(preferences_pass_len(pass->valuestring) < PREFERENCES_LORA_PASS_MIN){
				ESP_LOGE(TAG, "LoRa password must be at least %d characters", PREFERENCES_LORA_PASS_MIN);
				cJSON_Delete(json);
				return ESP_FAIL;
			}
			preferences_store_pass(pass->valuestring);
			temp.lora_pass = lora_pass_buf;
			temp.lora_key = preferences_key_from_pass(lora_pass_buf);
		} else {
			temp.lora_pass = lora_pass_buf;
			temp.lora_key = preferences_parse_lora_key(cJSON_GetObjectItem(json, "lora_key"));
		}
	}

	return Preferences_update(temp, true);
}

static uint32_t preferences_rotr(uint32_t x, uint32_t n) {
	return (x >> n) | (x << (32 - n));
}

static void preferences_sha256_compress(uint32_t h[8], const uint32_t *K, const uint8_t block[64]) {
	uint32_t w[64];
	for(int i = 0; i < 16; i++){
		w[i] = ((uint32_t)block[i * 4] << 24) | ((uint32_t)block[i * 4 + 1] << 16)
		     | ((uint32_t)block[i * 4 + 2] << 8) | (uint32_t)block[i * 4 + 3];
	}
	for(int i = 16; i < 64; i++){
		uint32_t s0 = preferences_rotr(w[i - 15], 7) ^ preferences_rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
		uint32_t s1 = preferences_rotr(w[i - 2], 17) ^ preferences_rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
		w[i] = w[i - 16] + s0 + w[i - 7] + s1;
	}
	uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
	uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];
	for(int i = 0; i < 64; i++){
		uint32_t S1 = preferences_rotr(e, 6) ^ preferences_rotr(e, 11) ^ preferences_rotr(e, 25);
		uint32_t ch = (e & f) ^ ((~e) & g);
		uint32_t t1 = hh + S1 + ch + K[i] + w[i];
		uint32_t S0 = preferences_rotr(a, 2) ^ preferences_rotr(a, 13) ^ preferences_rotr(a, 22);
		uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
		uint32_t t2 = S0 + maj;
		hh = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
	}
	h[0] += a; h[1] += b; h[2] += c; h[3] += d;
	h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
}

/* SHA-256("abc") is ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad. */
static void preferences_sha256(const uint8_t *msg, size_t len, uint8_t out[32]) {
	static const uint32_t K[64] = {
		0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
		0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
		0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
		0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
		0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
		0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
		0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
		0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
	};
	uint32_t h[8] = {
		0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
		0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19
	};
	uint8_t block[64];
	size_t off = 0;
	uint64_t bits = (uint64_t)len * 8;

	while(len - off >= 64){
		preferences_sha256_compress(h, K, msg + off);
		off += 64;
	}

	size_t rem = len - off;
	memset(block, 0, sizeof(block));
	if(rem > 0)
		memcpy(block, msg + off, rem);
	block[rem] = 0x80;
	if(rem >= 56){
		preferences_sha256_compress(h, K, block);
		memset(block, 0, sizeof(block));
	}
	for(int i = 0; i < 8; i++)
		block[63 - i] = (uint8_t)(bits >> (8 * i));
	preferences_sha256_compress(h, K, block);

	for(int i = 0; i < 8; i++){
		out[i * 4]     = (uint8_t)(h[i] >> 24);
		out[i * 4 + 1] = (uint8_t)(h[i] >> 16);
		out[i * 4 + 2] = (uint8_t)(h[i] >> 8);
		out[i * 4 + 3] = (uint8_t)h[i];
	}
}

static size_t preferences_pass_len(const char *pass) {
	const char *start = pass ? pass : "";
	while(*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n')
		start++;
	size_t n = strlen(start);
	while(n > 0 && (start[n - 1] == ' ' || start[n - 1] == '\t' || start[n - 1] == '\r' || start[n - 1] == '\n'))
		n--;
	return n;
}

static void preferences_store_pass(const char *pass) {
	char tmp[sizeof(lora_pass_buf)];
	const char *start = pass ? pass : "";
	while(*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n')
		start++;
	size_t n = strlen(start);
	while(n > 0 && (start[n - 1] == ' ' || start[n - 1] == '\t' || start[n - 1] == '\r' || start[n - 1] == '\n'))
		n--;
	if(n > sizeof(tmp) - 1)
		n = sizeof(tmp) - 1;
	memcpy(tmp, start, n);
	tmp[n] = '\0';
	memcpy(lora_pass_buf, tmp, n + 1);
}

static uint64_t preferences_key_from_pass(const char *pass) {
	if(pass == NULL || strlen(pass) < PREFERENCES_LORA_PASS_MIN)
		return PREFERENCES_LORA_KEY_FACTORY;

	uint8_t hash[32];
	preferences_sha256((const uint8_t *)pass, strlen(pass), hash);
	uint64_t key = 0;
	for(int i = 0; i < 8; i++)
		key |= (uint64_t)hash[i] << (8 * i);
	if(key == 0)
		return PREFERENCES_LORA_KEY_FACTORY;
	return key;
}

static uint64_t preferences_parse_lora_key(const cJSON *item) {
	uint64_t key = PREFERENCES_LORA_KEY_FACTORY;

	if(cJSON_IsString(item) && item->valuestring != NULL){
		char *end = NULL;
		errno = 0;
		unsigned long long parsed = strtoull(item->valuestring, &end, 10);
		if(end != item->valuestring && end != NULL && *end == '\0' && errno != ERANGE)
			key = (uint64_t)parsed;
	} else if(cJSON_IsNumber(item) && item->valuedouble >= 0){
		key = (uint64_t)item->valuedouble;
	}

	/* 0 was written before this setting was used. Keep the previous link. */
	if(key == 0)
		key = PREFERENCES_LORA_KEY_FACTORY;

	return key;
}

static void preferences_add_lora_key(cJSON *json, uint64_t key) {
	char key_str[21];
	snprintf(key_str, sizeof(key_str), "%llu", (unsigned long long)key);
	cJSON_AddStringToObject(json, "lora_key", key_str);
	cJSON_AddStringToObject(json, "key", key_str);
}

static int preferences_tracker_khz(int khz) {
	switch (khz) {
	case 434250:
	case 434375:
	case 434500:
	case 434625:
		return khz;
	default:
		return 434250;
	}
}

/**
 * @brief Checks if provided configuration JSON is valid. Pointer must not be NULL
 * and all necessary data must be present.
 *
 * @param json Pointer to configuration JSON to be check
 * @return bool True = JSON valid, False = invalid
 */
static bool json_check_cfg(cJSON * json){
	if(json == NULL) {
		return false;
	}

	bool json_check = false;
	json_check |= (NULL == cJSON_GetObjectItem(json, "main_alt"));
	json_check |= (NULL == cJSON_GetObjectItem(json, "max_tilt"));
	json_check |= (NULL == cJSON_GetObjectItem(json, "staging_delay"));
	json_check |= (NULL == cJSON_GetObjectItem(json, "staging_max_tilt"));
	json_check |= (NULL == cJSON_GetObjectItem(json, "rail_height"));
	json_check |= (NULL == cJSON_GetObjectItem(json, "auto_arming"));
	json_check |= (NULL == cJSON_GetObjectItem(json, "auto_arming_time_s"));
	json_check |= (NULL == cJSON_GetObjectItem(json, "lora_freq"));
	json_check |= (NULL == cJSON_GetObjectItem(json, "lora_mode"));
	{
		cJSON *lora_key = cJSON_GetObjectItem(json, "lora_key");
		cJSON *lora_pass = cJSON_GetObjectItem(json, "lora_pass");
		bool key_ok = cJSON_IsString(lora_key) || cJSON_IsNumber(lora_key);
		bool pass_ok = cJSON_IsString(lora_pass);
		json_check |= !(key_ok || pass_ok);
		if(lora_pass != NULL)
			json_check |= !pass_ok;
	}
	json_check |= (NULL == cJSON_GetObjectItem(json, "wifi_pass"));
	json_check |= (NULL == cJSON_GetObjectItem(json, "drouge_alt"));
	json_check |= (NULL == cJSON_GetObjectItem(json, "key"));

	if(!json_check)
		json_check |= !cJSON_IsString(cJSON_GetObjectItem(json, "wifi_pass"));

	return !json_check;
}
