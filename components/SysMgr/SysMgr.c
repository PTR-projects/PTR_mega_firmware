#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "LED_driver.h"
#include "esp_err.h"
#include "SysMgr.h"

static const char *TAG = "SysMgr";

QueueHandle_t queue_SysMgrCheckout = NULL;
QueueHandle_t queue_SysMgrArming;

sysmgr_checkout_status_t 	sysmgr_checkout_status_d;
sysmgr_arming_state_t		sysmgr_arming_state_d;

esp_err_t SysMgr_init(){
	sysmgr_checkout_status_d.sysmgr  = check_void;
	sysmgr_checkout_status_d.main 	 = check_void;
	sysmgr_checkout_status_d.storage = check_void;
	sysmgr_checkout_status_d.lora 	 = check_void;
	sysmgr_checkout_status_d.analog  = check_void;
	sysmgr_checkout_status_d.utils 	 = check_void;
	sysmgr_checkout_status_d.web 	 = check_void;
	sysmgr_checkout_status_d.gnss 	 = check_void;

	queue_SysMgrCheckout = xQueueCreate( 100, sizeof( sysmgr_checkout_msg_t ) );
	if(queue_SysMgrCheckout == 0){
		sysmgr_checkout_status_d.sysmgr  = check_fail;
		return ESP_FAIL;
	}

	queue_SysMgrArming 	 = xQueueCreate( 100, sizeof( sysmgr_arming_state_t ) );
	if(queue_SysMgrArming == 0){
		sysmgr_checkout_status_d.sysmgr  = check_fail;
		return ESP_FAIL;
	}

	sysmgr_arming_state_d 			 = system_dissarmed;
	sysmgr_checkout_status_d.sysmgr  = check_ready;
	return ESP_OK;
}


esp_err_t SysMgr_checkout(sysmgr_checkout_component_t component, sysmgr_checkout_state_t state){
	sysmgr_checkout_msg_t tmp;

	tmp.component = component;
	tmp.state 	  = state;

	while(queue_SysMgrCheckout == NULL) {}	//wait for SysMgr to init

	if(xQueueSend(queue_SysMgrCheckout, (void *)&tmp, 0) != pdTRUE){
		ESP_LOGE(TAG, "SysMgr Checkout Queue too full to write new Msg!");
	}

	return ESP_OK;
}

esp_err_t SysMgr_update(){
	sysmgr_checkout_msg_t tmp;
	while(xQueueReceive(queue_SysMgrCheckout, &tmp, 1)){
		sysmgr_checkout_status_d.table[tmp.component] = tmp.state;
	}

	while(xQueueReceive(queue_SysMgrArming, &sysmgr_arming_state_d, 1)) {}

	return ESP_OK;
}

sysmgr_checkout_state_t SysMgr_getCheckoutStatus(){
	uint8_t components_to_check = sizeof(sysmgr_checkout_status_t) / sizeof(sysmgr_checkout_state_t);
	uint8_t sum = 0;

	while(components_to_check--){
		if(sysmgr_checkout_status_d.table[components_to_check] == 0)
			return check_fail;
		sum = sum | sysmgr_checkout_status_d.table[components_to_check];
	}

	if(sum & 0x04)
		return check_fail;

	if(sum & 0x02)
		return check_void;

	if(sum & 0x01)
		return check_ready;

	return check_fail;
}
sysmgr_checkout_state_t SysMgr_getComponentState(sysmgr_checkout_component_t components_to_check){
	return sysmgr_checkout_status_d.table[components_to_check];
}

esp_err_t SysMgr_setArm(sysmgr_arming_state_t state){
	if(xQueueSend(queue_SysMgrArming, (void *)&state, 0) != pdTRUE){
		ESP_LOGE(TAG, "SysMgr Arming Queue too full to write new Msg!");
	}

	return ESP_OK;
}

sysmgr_arming_state_t SysMgr_getArm(){
	return sysmgr_arming_state_d;
}
