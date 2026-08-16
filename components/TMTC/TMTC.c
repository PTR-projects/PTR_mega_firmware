#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_err.h"

#include "LORA_driver.h"
#include "PTR_DataPacket.h"
#include "Cansat_driver.h"
#include "TMTC.h"

static const char *TAG = "TMTC";

static StaticQueue_t  s_tx_queue_struct;
static uint8_t        s_tx_queue_buf[sizeof(kppacket_t)];
static QueueHandle_t  s_tx_queue;

static void tmtc_dispatch_rx(uint8_t *buf, uint8_t size);

esp_err_t TMTC_init(void) {
    s_tx_queue = xQueueCreateStatic(1, sizeof(kppacket_t),
                                    s_tx_queue_buf, &s_tx_queue_struct);
    if(s_tx_queue == NULL){
        ESP_LOGE(TAG, "Failed to create TX queue");
        return ESP_FAIL;
    }
    LORA_startRX();
    return ESP_OK;
}

void TMTC_send(const kppacket_t *pkt) {
    xQueueOverwrite(s_tx_queue, pkt);
}

void TMTC_process(void) {
    uint8_t rx_buf[256];
    uint8_t rx_size = 0;

    if(LORA_receive(rx_buf, &rx_size) == ESP_OK)
        tmtc_dispatch_rx(rx_buf, rx_size);

    kppacket_t tx_pkt;
    if(xQueueReceive(s_tx_queue, &tx_pkt, 0) == pdTRUE)
        LORA_sendWithLBT((uint8_t *)&tx_pkt.legacyheader, tx_pkt.packet_len);
}

static void tmtc_dispatch_rx(uint8_t *buf, uint8_t size) {
    kppacket_header_t *hdr = (kppacket_header_t *)buf;

    switch(hdr->packet_id.msg_type){
        case PACKET_HEARTBEAT:
            ESP_LOGI(TAG, "HB from 0x%04X", hdr->sender_id);
            break;
        case PACKET_CUSTOM_16B:
            Cansat_parsePacket(
                (kppacket_payload_cansat_t *)(buf + sizeof(kppacket_header_t)));
            break;
        default:
            ESP_LOGW(TAG, "Unhandled packet type 0x%02X", hdr->packet_id.msg_type);
            break;
    }
}
