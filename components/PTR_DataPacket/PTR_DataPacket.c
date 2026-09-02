#include <stdint.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"

#include "encryption.h"
#include "PTR_DataPacket.h"

#if TARGET_ESP
#include "esp_random.h"
// Hardware AES API

#else
// Software AES API
#endif

static const char *TAG = "PTR_DataPacket";

static void    encrypt_msg  (kppacket_t * msg);
static uint8_t getRandomByte();

void DataPacket_init(){
    #if TARGET_ESP

    #else
    srand(134);
    #endif
}

int8_t DataPacket_build_msg(kppacket_t * msg, msg_type_e msg_type, bool encrypted, uint8_t sender_id, uint8_t dest_id, uint16_t packet_no, uint32_t timestamp_ms, void * payload, uint8_t payload_len){
    // Checks
    if(msg == NULL)
        return -1;

    if(payload_len > (255 - sizeof(kppacket_header_t)))
        return -1;

    if(payload_len > 0 && payload == NULL)
        return -1;

    packet_id_t packet_id;
    packet_id.msg_ver    = 0;
    packet_id.retransmit = 0;
    packet_id.encoded    = encrypted;
    packet_id.msg_type   = msg_type;

    msg->header.packet_id = packet_id;
    
    msg->header.sender_id   = sender_id;
    msg->header.dest_id     = dest_id;
    msg->header.packet_no   = packet_no;
    msg->header.timestamp_ms = timestamp_ms;

    // Check payload length
    uint8_t expected_payload_len = 0;
    uint8_t expected_header_len = 0;
    switch(msg_type){
        case PACKET_HEARTBEAT:
            expected_payload_len = 0;
            expected_header_len  = sizeof(kppacket_header_t);
            break;
        case PACKET_LEGACY_FULL:
            expected_payload_len = sizeof(kppacket_payload_rocket_t);
            expected_header_len  = sizeof(kppacket_header_t);
            break;
        case PACKET_SENSORS:
            expected_payload_len = sizeof(kppacket_payload_rocket_meas_t);
            expected_header_len  = sizeof(kppacket_header_t);
            break;
        case PACKET_ADCS:
            expected_payload_len = sizeof(kppacket_payload_rocket_ADCS_t);
            expected_header_len  = sizeof(kppacket_header_t);
            break;
        case PACKET_TRACKER:
            expected_payload_len = sizeof(kppacket_payload_rocket_tracker_t);
            expected_header_len  = sizeof(kppacket_header_t);
            break;
		case PACKET_RECU_TC:
            expected_payload_len = sizeof(kppacket_recu_tc_t);
            expected_header_len  = sizeof(kppacket_header_t);
            break;
		case PACKET_RECU_TM:
            expected_payload_len = sizeof(kppacket_recu_tm_t);
            expected_header_len  = sizeof(kppacket_header_t);
            break;
        case PACKET_CUSTOM_8B:
            expected_payload_len = 8;
            expected_header_len  = sizeof(kppacket_header_t);
            break;
        case PACKET_CUSTOM_16B:
            expected_payload_len = 16;
            expected_header_len  = sizeof(kppacket_header_t);
            break;
        case PACKET_CUSTOM_32B:
            expected_payload_len = 32;
            expected_header_len = sizeof(kppacket_header_t);
            break;
        case PACKET_CUSTOM_64B:
            expected_payload_len = 64;
            expected_header_len  = sizeof(kppacket_header_t);
            break;
        case PACKET_CUSTOM_128B:
            expected_payload_len = 128;
            expected_header_len  = sizeof(kppacket_header_t);
            break;
        case PACKET_CUSTOM_240B:
            expected_payload_len = 240;
            expected_header_len  = sizeof(kppacket_header_t);
            break;
        default:
            return -1;
    }

    if(expected_payload_len != payload_len)
        return -1;

    uint8_t payload_offset = 0;
    if(encrypted){
        payload_offset = 4;
    }
    if(payload_len != 0)
        memcpy(msg->payload + payload_offset, payload, payload_len);

    msg->packet_len = expected_payload_len + expected_header_len + payload_offset;

    if(encrypted){
        encrypt_msg(msg);
    }

    return 0;
}

static void encrypt_msg(kppacket_t * msg){

    uint8_t * enc_header_ptr = msg->payload;
    struct {
        uint16_t random_2byte;
        uint16_t const_2byte;
    } encryption_header;

    encryption_header.const_2byte = 0xabcd;
    encryption_header.random_2byte = getRandomByte() | (getRandomByte()<<8);

    // Add security header at the beginning of the payload
    memcpy(enc_header_ptr, &encryption_header, sizeof(encryption_header));


    // Encryption Magic
    // TODO

}

static uint8_t getRandomByte(){
    #if TARGET_ESP
    // use ESP random generator
    return esp_random() % 255;

    #else
    // Use Std C Pseudo Random Generator
    return rand() % 255;
    #endif
    
}