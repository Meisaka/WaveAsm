#ifndef WAVEASM_HASH_INCL
#define WAVEASM_HASH_INCL

#include <stdint.h>
#include <stdlib.h>

uint32_t wva_murmur3(const void * key, size_t len);

struct wvas_hashentry {
	uint32_t keylen;
	char * key;
	uint32_t datalen;
	void * data;
	struct wvas_hashentry * next;
};

typedef struct wvas_arrayhash {
	uint32_t tablesize;
	uint32_t usedslots;
	uint32_t entries;
	struct wvas_hashentry ** table;
} *wvat_hashtab;

int wva_allochash(wvat_hashtab*);
void wva_freehash(wvat_hashtab);

#endif

