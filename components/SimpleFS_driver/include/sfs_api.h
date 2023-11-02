/*
 * sfs_api.h
 *
 *  Created on: 7 paź 2023
 *      Author: Bartek
 */

#ifndef COMPONENTS_SIMPLEFS_DRIVER_INCLUDE_SFS_API_H_
#define COMPONENTS_SIMPLEFS_DRIVER_INCLUDE_SFS_API_H_

#include <stdint.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint32_t partition_size_B;
	uint32_t partition_page_B;
} sfs_info_t;


esp_err_t simplefs_api_init(sfs_info_t * partition_info, const char * label);

/**
 * @brief Read a region in a block.
 *
 * Negative error codes are propogated to the user.
 *
 * @return errorcode. 0 on success.
 */
esp_err_t simplefs_api_read(uint32_t position, void *buffer, uint32_t size);

/**
 * @brief Program a region in a block.
 *
 * The block must have previously been erased.
 * Negative error codes are propogated to the user.
 * May return LFS_ERR_CORRUPT if the block should be considered bad.
 *
 * @return errorcode. 0 on success.
 */
esp_err_t simplefs_api_prog(uint32_t position, void *buffer, uint32_t size);

/**
 * @brief Erase a block.
 *
 * A block must be erased before being programmed.
 * The state of an erased block is undefined.
 * Negative error codes are propogated to the user.
 * May return LFS_ERR_CORRUPT if the block should be considered bad.
 * @return errorcode. 0 on success.
 */
esp_err_t simplefs_api_erase(uint32_t range_end_B);

#ifdef __cplusplus
}
#endif

#endif /* COMPONENTS_SIMPLEFS_DRIVER_INCLUDE_SFS_API_H_ */
