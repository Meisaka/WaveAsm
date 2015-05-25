#include <stdint.h>
#include <stdlib.h>

// MurmurHash3 was written by Austin Appleby, and is public domain.
// see http://code.google.com/p/smhasher/

#define ROTL32(x,y) ((x << y) | (x >> (32 - y)))

uint32_t wva_murmur3(const void * key, size_t len)
{
	const uint8_t * data = (const uint8_t*)key;
	const size_t nblocks = len / 4;

	uint32_t h1 = 0;

	const uint32_t c1 = 0xcc9e2d51;
	const uint32_t c2 = 0x1b873593;

	//----------
	// body

	const uint32_t * blocks = (const uint32_t *)(data);

	size_t i;
	for(i = nblocks; i; i--) {
		uint32_t k1 = *(blocks++);

		k1 *= c1;
		k1 = ROTL32(k1,15);
		k1 *= c2;

		h1 ^= k1;
		h1 = ROTL32(h1,13);
		h1 = h1 * 5 + 0xe6546b64;
	}

	//----------
	// tail

	const uint8_t * tail = (const uint8_t*)(blocks);

	uint32_t k1 = 0;

	switch(len & 3) {
	case 3: k1 ^= tail[2] << 16;
	case 2: k1 ^= tail[1] << 8;
	case 1: k1 ^= tail[0];
		k1 *= c1;
		k1 = ROTL32(k1,15);
		k1 *= c2;
		h1 ^= k1;
	}

	//----------
	// finalization

	h1 ^= len;

	h1 ^= h1 >> 16;
	h1 *= 0x85ebca6b;
	h1 ^= h1 >> 13;
	h1 *= 0xc2b2ae35;
	h1 ^= h1 >> 16;

	return h1;
}

