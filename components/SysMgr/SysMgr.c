#include <stdio.h>
//#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "LED_driver.h"
#include "SysMgr.h"

static const char *TAG = "SysMgr";

QueueHandle_t queue_SysMgrCheckout;
sysmgr_checkout_status_t sysmgr_checkout_status_d;

esp_err_t SysMgr_init(){
	sysmgr_checkout_status_d.sysmgr  = check_void;
	sysmgr_checkout_status_d.main 	 = check_void;
	sysmgr_checkout_status_d.storage = check_void;
	sysmgr_checkout_status_d.lora 	 = check_void;
	sysmgr_checkout_status_d.analog  = check_void;
	sysmgr_checkout_status_d.utils 	 = check_void;
	sysmgr_checkout_status_d.web 	 = check_void;

	queue_SysMgrCheckout = xQueueCreate( 100, sizeof( sysmgr_checkout_msg_t ) );

	if(queue_SysMgrCheckout == 0){
		sysmgr_checkout_status_d.sysmgr  = check_fail;
		return ESP_FAIL;
	}

	sysmgr_checkout_status_d.sysmgr  = check_ready;
	return ESP_OK;
}


esp_err_t SysMgr_checkout(sysmgr_checkout_component_t component, sysmgr_checkout_state_t state){
	sysmgr_checkout_msg_t tmp;

	tmp.component = component;
	tmp.state 	  = state;
	if(xQueueSend(queue_SysMgrCheckout, (void *)&tmp, 0) != pdTRUE){
		ESP_LOGE(TAG, "SysMgr Queue to full to write new Msg!");
	}

	return ESP_OK;
}

esp_err_t SysMgr_update(){
	sysmgr_checkout_msg_t tmp;
	while(xQueueReceive(queue_SysMgrCheckout, &tmp, 1)){
		sysmgr_checkout_status_d.table[tmp.component] = tmp.state;
	}

	return ESP_OK;
}

sysmgr_checkout_state_t SysMgr_getCheckoutStatus(){
	uint8_t components_to_check = sizeof(sysmgr_checkout_state_t) / sizeof(sysmgr_checkout_state_t);
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
