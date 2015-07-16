#ifndef WAVEASM_HASH_INCL
#define WAVEASM_HASH_INCL

#ifdef __cplusplus
#define WAVEASMC extern "C"
#else
#define WAVEASMC
#endif

#include <stdint.h>
#include <stdlib.h>

WAVEASMC
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

struct wvas_hashentry* wva_lookuphash(wvat_hashtab, const char *, uint32_t);
int wva_addhash(wvat_hashtab, const char *, uint32_t, const uint8_t *, uint32_t);
int wva_allochash(wvat_hashtab*);
void wva_freehash(wvat_hashtab);

#endif

