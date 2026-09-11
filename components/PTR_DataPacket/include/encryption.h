#pragma once

#include <stdint.h>

#define ENCRYPTION_OK       0
#define ENCRYPTION_ERR_ARG -1
#define ENCRYPTION_ERR_KEY -2

/* 64-bit key. XXTEA's schedule has four uint32 words; the two 32-bit
 * halves of key are copied into all four slots, so entropy is 64 bits.
 */
int Encryption_init(uint64_t key);

void Encryption_encode(uint8_t *v, int size);
void Encryption_decode(uint8_t *v, int size);