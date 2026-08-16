#include <stdint.h>
#include "encryption.h"

static void btea(uint8_t *v, int size, uint64_t key64);
static uint64_t encryption_key = 0x123456123456;

void Encryption_init(uint64_t key){
    encryption_key = key;
}

void Encryption_encode(uint8_t *v, int size){
    btea(v,  size, encryption_key);
}

void Encryption_decode(uint8_t *v, int size){
    btea(v,  -size, encryption_key);
}

/* XXTEA encryption algorithm
*  Source: https://en.wikipedia.org/wiki/XXTEA
*/
static void btea(uint8_t *v, int size, uint64_t key64) {
	#define DELTA 0x9e3779b9
	#define MX (((z>>5^y<<2) + (y>>3^z<<4)) ^ ((sum^y) + (key[(p&3)^e] ^ z)))
	uint32_t  key[4] = {0};
    uint32_t y, z, sum;
    unsigned p, rounds, e;

	/* Build key */
	for(uint32_t i=0; i<4; i++){
		key[i] = *(((uint16_t*)(&key64))+i);
		key[i] |= key[i]<<16;
	}

	/* Encoding Part */
    if (size > 1) {
		rounds = 6 + 52/size;
		sum = 0;
		z = v[size-1];
		do {
			sum += DELTA;
			e = (sum >> 2) & 3;
			for (p=0; p<size-1; p++) {
				y = v[p+1];
				z = v[p] += MX;
			}
			y = v[0];
			z = v[size-1] += MX;
		} while (--rounds);

		return;
    }

	/* Decoding Part */
	if (size < -1) {
		size = -size;
		rounds = 6 + 52/size;
		sum = rounds*DELTA;
		y = v[0];
		do {
			e = (sum >> 2) & 3;
			for (p=size-1; p>0; p--) {
				z = v[p-1];
				y = v[p] -= MX;
			}
			z = v[size-1];
			y = v[0] -= MX;
			sum -= DELTA;
		} while (--rounds);

	return;
    }
}