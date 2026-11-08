#include <stdint.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"

#include "encryption.h"
#include "PTR_DataPacket.h"

#if TARGET_ESP
#include "esp_random.h"
#include "esp_mac.h"
// Hardware AES API

#else
// Software AES API
#endif

static const char *TAG = "PTR_DataPacket";

static uint32_t s_sender_id     = 0;
static uint16_t s_sender_id_ext = 0;
static uint64_t s_target_id     = 0;
static bool     s_broadcast_unlocked = false;

static void     encrypt_msg  (kppacket_t * msg, uint8_t length);
static bool     decrypt_msg  (kppacket_t * msg, uint8_t length);
static uint8_t  getRandomByte();
static uint16_t crc16        (uint8_t *buf, uint32_t len);

static uint64_t datapacket_pack_target_id(uint16_t id_ext, uint32_t id) {
    uint64_t board_id_48 = ((uint64_t)id_ext << 32) | (uint64_t)id;
    uint16_t hash16 = (uint16_t)(board_id_48 >> 32)
                    ^ (uint16_t)(board_id_48 >> 16)
                    ^ (uint16_t)board_id_48;
    return (board_id_48 << 16) | (uint64_t)hash16;
}

void DataPacket_init(){
    s_broadcast_unlocked = false;

    #if TARGET_ESP
    uint8_t mac[6] = {0};
    if(esp_read_mac(mac, ESP_MAC_WIFI_STA) == ESP_OK){
        /* MAC AA:BB:CC:DD:EE:FF → ext=AABB, id=CCDDEEFF */
        s_sender_id_ext = ((uint16_t)mac[0] << 8) | (uint16_t)mac[1];
        s_sender_id     = ((uint32_t)mac[2] << 24) | ((uint32_t)mac[3] << 16)
                        | ((uint32_t)mac[4] << 8)  | (uint32_t)mac[5];
        s_target_id     = datapacket_pack_target_id(s_sender_id_ext, s_sender_id);
        ESP_LOGI(TAG, "LoRa sender ID 0x%04X%08lX target 0x%016llX (MAC %02X:%02X:%02X:%02X:%02X:%02X)",
                 s_sender_id_ext, (unsigned long)s_sender_id,
                 (unsigned long long)s_target_id,
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    } else {
        ESP_LOGW(TAG, "esp_read_mac failed; LoRa sender ID remains 0");
        s_target_id = datapacket_pack_target_id(0, 0);
    }
    #else
    srand(134);
    s_target_id = datapacket_pack_target_id(s_sender_id_ext, s_sender_id);
    #endif

    Encryption_init(12345);
}

uint32_t DataPacket_get_sender_id(void) {
    return s_sender_id;
}

uint16_t DataPacket_get_sender_id_ext(void) {
    return s_sender_id_ext;
}

void DataPacket_set_sender_id(uint16_t sender_id_ext, uint32_t sender_id) {
    s_sender_id_ext = sender_id_ext;
    s_sender_id     = sender_id;
    s_target_id     = datapacket_pack_target_id(s_sender_id_ext, s_sender_id);
    ESP_LOGI(TAG, "LoRa sender ID overridden to 0x%04X%08lX target 0x%016llX",
             s_sender_id_ext, (unsigned long)s_sender_id,
             (unsigned long long)s_target_id);
}

uint64_t DataPacket_get_target_id(void) {
    return s_target_id;
}

bool DataPacket_target_id_matches(uint64_t target_id) {
    if(target_id == s_target_id)
        return true;
    if(target_id == DATAPACKET_TARGET_ID_BROADCAST && s_broadcast_unlocked)
        return true;
    return false;
}

void DataPacket_broadcast_unlock(void) {
    s_broadcast_unlocked = true;
}

void DataPacket_broadcast_lock(void) {
    s_broadcast_unlocked = false;
}

bool DataPacket_broadcast_is_unlocked(void) {
    return s_broadcast_unlocked;
}

int8_t DataPacket_build_msg(kppacket_t * msg, msg_type_e msg_type, bool encrypted, uint16_t packet_no, uint32_t timestamp_ms, void * payload, uint8_t payload_len){
    // Checks
    if(msg == NULL)
        return -1;

    if(payload_len > (255 - sizeof(kppacket_header_t)))
        return -1;

    if(payload_len > 0 && payload == NULL)
        return -1;

    // Heartbeat has no payload — nothing to encrypt.
    if(encrypted && (msg_type == PACKET_HEARTBEAT || payload_len == 0))
        return -1;

    // Old frame - must not be used!
    if(msg_type == PACKET_LEGACY_FULL)
        return -1;

    packet_id_t packet_id;
    packet_id.msg_ver    = 0;
    packet_id.retransmit = 0;
    packet_id.encoded    = encrypted;
    packet_id.msg_type   = msg_type;
    packet_id.redu       = 0;
    packet_id.command    = (msg_type == PACKET_RECU_TC || msg_type == PACKET_CUSTOM_32B) ? 1 : 0;

    msg->header.packet_id     = packet_id;
    msg->header.sender_id     = s_sender_id;
    msg->header.sender_id_ext = s_sender_id_ext;
    msg->header.packet_no     = packet_no;
    msg->header.timestamp_ms  = timestamp_ms;

    // Set header and payload length
    uint8_t expected_payload_len = 0;
    uint8_t expected_header_len  = 0;
    switch(msg_type){
        case PACKET_HEARTBEAT:
            expected_payload_len = 0;
            expected_header_len  = sizeof(kppacket_header_t);
            break;
         case PACKET_ROCKET_FULL:
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
            expected_header_len  = sizeof(kppacket_header_t);
            break;
        case PACKET_CUSTOM_64B:
            expected_payload_len = 64;
            expected_header_len  = sizeof(kppacket_header_t);
            break;
        case PACKET_CUSTOM_128B:
            expected_payload_len = 128;
            expected_header_len  = sizeof(kppacket_header_t);
            break;
        case PACKET_CUSTOM_235B:
            expected_payload_len = 235;
            expected_header_len  = sizeof(kppacket_header_t);
            break;
        default:
            return -1;
    }

    if(expected_payload_len != payload_len)
        return -1;

    if(payload_len != 0)
        memcpy(msg->payload, payload, payload_len);

    msg->packet_len = expected_payload_len + expected_header_len;

    if(encrypted){
        encrypt_msg(msg, payload_len);
    }

    return 0;
}

bool DataPacket_unpack_msg(kppacket_t * pMsg, uint8_t *buf, uint8_t size){
    if(buf == NULL)
        return false;
        
    if(size < sizeof(kppacket_header_t))
        return false;

    memcpy(&(pMsg->header), buf, size);
    pMsg->packet_len = size;

    if(pMsg->header.packet_id.encoded == true){
        if(decrypt_msg(pMsg, size - sizeof(kppacket_header_t)) == false)
            return false;
    }

    return true;
}

static void encrypt_msg(kppacket_t * msg, uint8_t length){
    uint8_t * pPayload = msg->payload;
    struct {
        uint16_t random_2byte;
        uint16_t payload_crc16;
    } encryption_header;

    // Add random word and CRC16 of the payload
    encryption_header.random_2byte  = getRandomByte() | (getRandomByte()<<8);
    encryption_header.payload_crc16 = crc16(pPayload, length);

    // Add security header at the end of the payload
    memcpy(pPayload + length, &encryption_header, sizeof(encryption_header));

    // Encryption Magic
    Encryption_encode(pPayload, length + sizeof(encryption_header));

    // Increase payload size by adding encryption header size
    msg->packet_len += sizeof(encryption_header);
}

static bool decrypt_msg(kppacket_t * msg, uint8_t length){
    if(length <= 4)
        return false;

    uint8_t * pPayload = msg->payload;
    struct {
        uint16_t random_2byte;
        uint16_t payload_crc16;
    } encryption_header;

    // Subtract encryption header length
    msg->packet_len -= 4;

    // Decryption magic
    Encryption_decode(pPayload, length);

    // Extract encryption header
    memcpy(&encryption_header, pPayload + length - sizeof(encryption_header), sizeof(encryption_header));

    // Check CRC16
    uint16_t calc_crc16 = crc16(pPayload, length - sizeof(encryption_header));
    if(calc_crc16 != encryption_header.payload_crc16)
        return false;

    return true;
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

const static uint16_t crc_tab16[256] =
{        
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040,
};

static uint16_t crc16(uint8_t *buf, uint32_t len){
	uint16_t crc = 0xFFFF;

	while(len--){
		crc = (crc >> 8) ^ crc_tab16[(crc ^ *(buf++)) & 0xff];
	}

	return crc;
}
