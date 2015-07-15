
#include "hash.h"
#include "wavefunc.h"
#include "string.h"

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

struct wvas_hashentry* wva_getentryhash(struct wvas_hashentry* hte, const char * key, uint32_t kl) {
	while(hte) {
		if(hte->keylen == kl && strncmp(key, hte->key, kl) == 0) {
			return hte;
		}
		hte = hte->next;
	}
	return 0;
}

struct wvas_hashentry* wva_lookuphash(wvat_hashtab t, const char * key, uint32_t kl) {
	uint32_t h;

	if(!t) return 0;
	h = wva_murmur3(key, kl) % t->tablesize;
	return wva_getentryhash(t->table[h], key, kl);
}

int wva_addhash(wvat_hashtab t, const char * key, uint32_t kl, const uint8_t * d, uint32_t dl) {
	uint32_t h;
	struct wvas_hashentry* hte;
	struct wvas_hashentry* htn;

	if(!t || !key || !kl) return 0;
	h = wva_murmur3(key, kl) % t->tablesize;
	hte = t->table[h];
	if(!hte) {
		hte = (struct wvas_hashentry*)wva_alloc(sizeof(struct wvas_hashentry));
		if(!hte) return 4;
		hte->next = 0;
		hte->key = (char*)wva_alloc(kl);
		if(!hte->key) {
			wva_free(hte);
			return 4;
		}
		hte->keylen = kl;
		hte->datalen = 0;
		hte->data = 0;
		memcpy(hte->key, key, kl);
	} else {
		htn = wva_getentryhash(hte, key, kl);
		if(htn) {
			hte = htn;
		} else {
			htn = hte;
			hte = (struct wvas_hashentry*)wva_alloc(sizeof(struct wvas_hashentry));
			if(!hte) return 4;
			hte->next = htn;
			hte->key = (char*)wva_alloc(kl);
			if(!hte->key) {
				wva_free(hte);
				return 4;
			}
			hte->keylen = kl;
			hte->datalen = 0;
			hte->data = 0;
			memcpy(hte->key, key, kl);
			t->table[h] = hte;
		}
	}
	if(!hte->datalen && dl) {
		hte->data = wva_alloc(dl);
		if(!hte->data) {
			wva_free(hte->key);
			wva_free(hte);
			return 4;
		}
	}
	if(!dl && hte->datalen) {
		wva_free(hte->data);
		hte->data = 0;
	} else if(dl && hte->datalen == dl) {
		memcpy(hte->data, d, dl);
	}
	return 0;
}

int wva_allochash(wvat_hashtab* nht) {
	wvat_hashtab th;
	void * arr;

	arr = wva_alloc(sizeof(struct wvas_arrayhash));
	if(!arr) return 4;
	th = (wvat_hashtab)arr;
	th->tablesize = 63;
	th->usedslots = 0;
	th->entries = 0;
	arr = wva_alloc(sizeof(void*) * th->tablesize);
	if(!arr) {
		wva_free(th);
		return 4;
	}
	int i;
	for(i = 0; i < th->tablesize; i++) {
		((void**)arr)[i] = 0;
	}
	th->table = (struct wvas_hashentry**)arr;
	*nht = th;
	return 0;
}
void wva_freehash(wvat_hashtab nht) {
	if(!nht || !nht->table) return;
	int i;
	struct wvas_hashentry* hte;
	struct wvas_hashentry* htn;
	for(i = 0; i < nht->tablesize; i++) {
		hte = nht->table[i];
		while(hte) {
			if(hte->key) wva_free(hte->key);
			if(hte->data) wva_free(hte->data);
			htn = hte->next;
			wva_free(hte);
			hte = htn;
		}
	}
	wva_free(nht->table);
	wva_free(nht);
}

