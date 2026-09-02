#pragma once

void Encryption_init  (uint64_t key);
void Encryption_encode(uint8_t *v, int size);
void Encryption_decode(uint8_t *v, int size);