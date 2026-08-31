#pragma once

#include <stdint.h>

#define ENCRYPTION_OK       0
#define ENCRYPTION_ERR_ARG -1
#define ENCRYPTION_ERR_KEY -2

/* 64-bit key. XXTEA's schedule has four uint32 words; the two 32-bit
 * halves of key are copied into all four slots, so entropy is 64 bits.
 */
int Encryption_init(uint64_t key);

int Encryption_encode(uint32_t *v, int n);
int Encryption_decode(uint32_t *v, int n);

int Encryption_encode_bytes(uint8_t *data, int size);
int Encryption_decode_bytes(uint8_t *data, int size);
