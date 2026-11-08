#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "esp_log.h"
#include "esp_err.h"
#include "BOARD_cfg.h"
#include "SD_driver.h"

#if !defined(SD_ENABLED)

esp_err_t SD_init(void){
	return ESP_OK;
}

esp_err_t SD_exportFlightLog(void){
	return ESP_ERR_NOT_SUPPORTED;
}

#else

#include "esp_crc.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "SimpleFS_driver.h"
#include "DataManager.h"

static const char *TAG = "SD_driver";

#define SD_MOUNT_POINT		"/sdcard"
#define SD_READ_CHUNK_B		4096

static sdmmc_card_t *s_card = NULL;
static bool s_mounted = false;

static const char *CSV_HEADER =
	"sys_time;"
	"accX;accY;accZ;"
	"gyroX;gyroY;gyroZ;"
	"magX;magY;magZ;"
	"accHX;accHY;accHZ;"
	"pressure;temp;"
	"latitude;longitude;altitude_gnss;gnss_sats;gnss_fix;"
	"altitude_press;altitude_kalman;ascent_rate_kalman;tilt;"
	"q0;q1;q2;q3;"
	"flightstate;"
	"ign1_cont;ign2_cont;ign3_cont;ign4_cont;"
	"ign1_state;ign2_state;ign3_state;ign4_state;"
	"vbat_mV;"
	"servo_1;servo_2;servo_3;servo_4;servo_en";

esp_err_t SD_init(void){
	if(s_mounted){
		return ESP_OK;
	}

	esp_vfs_fat_sdmmc_mount_config_t mount_config = {
		.format_if_mount_failed = false,
		.max_files = 5,
		.allocation_unit_size = 16 * 1024
	};

	sdmmc_host_t host = SDMMC_HOST_DEFAULT();
	sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
	slot_config.width = 4;
#if SOC_SDMMC_USE_GPIO_MATRIX
	slot_config.clk = SD_CLK_PIN;
	slot_config.cmd = SD_CMD_PIN;
	slot_config.d0  = SD_DAT0_PIN;
	slot_config.d1  = SD_DAT1_PIN;
	slot_config.d2  = SD_DAT2_PIN;
	slot_config.d3  = SD_DAT3_PIN;
#endif
	slot_config.cd = SD_DETECT_PIN;
	slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

	esp_err_t ret = esp_vfs_fat_sdmmc_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &s_card);
	if(ret != ESP_OK){
		if(ret == ESP_FAIL){
			ESP_LOGE(TAG, "Failed to mount filesystem");
		} else {
			ESP_LOGE(TAG, "Failed to initialize SD card: %s", esp_err_to_name(ret));
		}
		return ret;
	}

	s_mounted = true;
	ESP_LOGI(TAG, "SD card mounted at %s", SD_MOUNT_POINT);
	sdmmc_card_print_info(stdout, s_card);
	return ESP_OK;
}

static bool SD_pathExists(const char *path){
	struct stat st;
	return stat(path, &st) == 0;
}

/**
 * @brief Find next free NNN by probing %03u.bin / %03u.csv existence
 *        (same idea as FatFS FA_CREATE_NEW + FR_EXIST increment).
 */
static int SD_findNextIndex(void){
	for(int file_num = 0; file_num <= 999; file_num++){
		char bin_path[40];
		char csv_path[40];
		snprintf(bin_path, sizeof(bin_path), "%s/%03u.bin", SD_MOUNT_POINT, (unsigned)file_num);
		snprintf(csv_path, sizeof(csv_path), "%s/%03u.csv", SD_MOUNT_POINT, (unsigned)file_num);

		if(SD_pathExists(bin_path) || SD_pathExists(csv_path)){
			continue;
		}

		/* Confirm exclusivity the FatFS way: create-new must succeed */
		FILE *f = fopen(bin_path, "wbx");
		if(f == NULL){
			/* Race or unexpected; treat as taken and keep looking */
			continue;
		}
		fclose(f);
		unlink(bin_path);
		ESP_LOGI(TAG, "Next free file index: %03u", (unsigned)file_num);
		return file_num;
	}

	ESP_LOGE(TAG, "No free NNN index (000-999 full)");
	return -1;
}

static esp_err_t SD_writeBinFile(const char *path){
	FILE *f = fopen(path, "wb");
	if(f == NULL){
		ESP_LOGE(TAG, "fopen %s failed", path);
		return ESP_FAIL;
	}

	SimpleFS_readMode();
	SimpleFS_resetReadPointer();

	uint8_t *chunk = malloc(SD_READ_CHUNK_B);
	if(chunk == NULL){
		fclose(f);
		SimpleFS_writeMode();
		return ESP_ERR_NO_MEM;
	}

	esp_err_t err = ESP_OK;
	int32_t n;
	do {
		n = SimpleFS_readMemory(SD_READ_CHUNK_B, chunk);
		if(n < 0){
			ESP_LOGE(TAG, "SimpleFS_readMemory failed");
			err = ESP_FAIL;
			break;
		}
		if(n > 0){
			if(fwrite(chunk, 1, (size_t)n, f) != (size_t)n){
				ESP_LOGE(TAG, "fwrite bin failed");
				err = ESP_FAIL;
				break;
			}
		}
	} while(n > 0);

	free(chunk);
	fclose(f);
	SimpleFS_writeMode();
	return err;
}

static bool SD_packetValid(const sfs_packet_t *pkt){
	if(pkt->header.pre != SFS_HEADER_PRE){
		return false;
	}
	uint16_t crc = SimpleFS_crc16((uint8_t *)pkt, sizeof(sfs_packet_t) - sizeof(pkt->CRC16));
	return crc == pkt->CRC16;
}

static void SD_writeCsvLine(FILE *f, const DataPackage_t *p){
	uint8_t gnss_raw = (uint8_t)p->sensors.gnss_fix;
	uint8_t gnss_sats = gnss_raw & 0x3F;
	uint8_t gnss_fix = (gnss_raw >> 6) & 0x03;

	fprintf(f,
		"%lu;"
		"%.6g;%.6g;%.6g;"
		"%.6g;%.6g;%.6g;"
		"%.6g;%.6g;%.6g;"
		"%.6g;%.6g;%.6g;"
		"%.6g;%d;"
		"%.6g;%.6g;%.6g;%u;%u;"
		"%.6g;%.6g;%.6g;%u;"
		"%.6g;%.6g;%.6g;%.6g;"
		"%u;"
		"%u;%u;%u;%u;"
		"%u;%u;%u;%u;"
		"%u;"
		"%d;%d;%d;%d;%u\n",
		(unsigned long)p->sys_time,
		p->sensors.accX, p->sensors.accY, p->sensors.accZ,
		p->sensors.gyroX, p->sensors.gyroY, p->sensors.gyroZ,
		p->sensors.magX, p->sensors.magY, p->sensors.magZ,
		p->sensors.accHX, p->sensors.accHY, p->sensors.accHZ,
		p->sensors.pressure, (int)p->sensors.temp,
		p->sensors.latitude, p->sensors.longitude, p->sensors.altitude_gnss, gnss_sats, gnss_fix,
		p->ahrs.altitude_press, p->ahrs.altitude_kalman, p->ahrs.ascent_rate_kalman, p->ahrs.tilt,
		p->ahrs.q0, p->ahrs.q1, p->ahrs.q2, p->ahrs.q3,
		p->flightstate,
		p->ign.ign1_cont, p->ign.ign2_cont, p->ign.ign3_cont, p->ign.ign4_cont,
		p->ign.ign1_state, p->ign.ign2_state, p->ign.ign3_state, p->ign.ign4_state,
		p->vbat_mV,
		p->servo.servo_1, p->servo.servo_2, p->servo.servo_3, p->servo.servo_4, p->servo.servo_en
	);
}

static esp_err_t SD_writeCsvFile(const char *path){
	FILE *f = fopen(path, "w");
	if(f == NULL){
		ESP_LOGE(TAG, "fopen %s failed", path);
		return ESP_FAIL;
	}

	fprintf(f, "%s\n", CSV_HEADER);

	SimpleFS_readMode();
	SimpleFS_resetReadPointer();

	sfs_packet_t pkt;
	esp_err_t err = ESP_OK;
	int32_t n;
	do {
		n = SimpleFS_readMemory(sizeof(sfs_packet_t), &pkt);
		if(n < 0){
			ESP_LOGE(TAG, "SimpleFS_readMemory failed (csv)");
			err = ESP_FAIL;
			break;
		}
		if(n == 0){
			break;
		}
		if((size_t)n < sizeof(sfs_packet_t)){
			break;
		}
		if(!SD_packetValid(&pkt)){
			continue;
		}

		DataPackage_t pkg;
		memset(&pkg, 0, sizeof(pkg));
		size_t copy_len = sizeof(DataPackage_t);
		if(copy_len > sizeof(pkt.payload)){
			copy_len = sizeof(pkt.payload);
		}
		memcpy(&pkg, pkt.payload, copy_len);
		SD_writeCsvLine(f, &pkg);
	} while(n > 0);

	fclose(f);
	SimpleFS_writeMode();
	return err;
}

esp_err_t SD_exportFlightLog(void){
	if(!s_mounted){
		esp_err_t init_err = SD_init();
		if(init_err != ESP_OK){
			return init_err;
		}
	}

	int idx = SD_findNextIndex();
	if(idx < 0){
		return ESP_FAIL;
	}

	char bin_path[32];
	char csv_path[32];
	snprintf(bin_path, sizeof(bin_path), "%s/%03d.bin", SD_MOUNT_POINT, idx);
	snprintf(csv_path, sizeof(csv_path), "%s/%03d.csv", SD_MOUNT_POINT, idx);

	ESP_LOGI(TAG, "Exporting flight log to %s and %s (%lu bytes)",
		bin_path, csv_path, (unsigned long)SimpleFS_getFileSize());

	esp_err_t err = SD_writeBinFile(bin_path);
	if(err != ESP_OK){
		unlink(bin_path);
		return err;
	}

	err = SD_writeCsvFile(csv_path);
	if(err != ESP_OK){
		unlink(csv_path);
		return err;
	}

	ESP_LOGI(TAG, "Flight log export complete");
	return ESP_OK;
}

#endif /* SD_ENABLED */
