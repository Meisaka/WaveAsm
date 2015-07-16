/****
 * WaveAsm wave.c - core functions
 * Copyright (c) 2013-2015, Meisaka Yukara <Meisaka.Yukara at gmail dot com>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "setting.h"
#include "wave.h"
#include "wavefunc.h"
#include "hash.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <string>

int strneq(const char *a, size_t al, const char *b, size_t bl)
{
	if(al != bl) return 0;
	return strncmp(a, b, al) == 0;
}

// MurmurHash3 was written by Austin Appleby, and is public domain.
// see http://code.google.com/p/smhasher/
#define ROTL32(x,y) ((x << y) | (x >> (32 - y)))
extern "C"
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

namespace Wave {

struct hashentry {
	uint32_t keylen;
	char *key;
	uint32_t datalen;
	void *data;
	hashentry * next;
};

struct hashentry* wva_getentryhash(struct hashentry* hte, const char * key, uint32_t kl) {
	while(hte) {
		if(hte->keylen == kl && strncmp(key, hte->key, kl) == 0) {
			return hte;
		}
		hte = hte->next;
	}
	return 0;
}

template<typename T>
class arrayhash {
private:
	uint32_t tablesize;
	uint32_t usedslots;
	uint32_t entries;
	hashentry ** table;
public:
	hashentry* lookup(const char *key, uint32_t kl) { 
		uint32_t h;

		if(!this) return 0;
		h = wva_murmur3(key, kl) % this->tablesize;
		fprintf(stderr, "HT: H:%d P:%X\n", h, this->table[h]);
		return wva_getentryhash(this->table[h], key, kl);
	}
	int set(const char * key, uint32_t kl, const T &d) {
		uint32_t h;
		uint8_t *td;
		hashentry* hte;
		hashentry* htn;

		if(!this || !key || !kl) return 0;
		h = wva_murmur3(key, kl) % this->tablesize;
		hte = this->table[h];
		if(!hte) {
			hte = (struct hashentry*)wva_alloc(sizeof(struct hashentry));
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
			this->table[h] = hte;
		} else {
			htn = wva_getentryhash(hte, key, kl);
			if(htn) {
				hte = htn;
			} else {
				htn = hte;
				hte = (struct hashentry*)wva_alloc(sizeof(struct hashentry));
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
				this->table[h] = hte;
			}
		}
		if(!hte->datalen) {
			hte->data = new T(d);
			hte->datalen = sizeof(T);
		} else {
			delete (T*)hte->data;
			hte->data = new T(d);
			hte->datalen = sizeof(T);
		}
		return 0;
	}
	arrayhash() {
		void *arr;

		this->tablesize = 63;
		this->usedslots = 0;
		this->entries = 0;
		arr = wva_alloc(sizeof(void*) * this->tablesize);
		if(!arr) {
			return;
		}
		int i;
		for(i = 0; i < this->tablesize; i++) {
			((void**)arr)[i] = 0;
		}
		this->table = (struct hashentry**)arr;
	}
	~arrayhash() {
		if(!this || !this->table) return;
		int i;
		struct hashentry* hte;
		struct hashentry* htn;
		for(i = 0; i < this->tablesize; i++) {
			hte = this->table[i];
			while(hte) {
				if(hte->key) wva_free(hte->key);
				if(hte->data) delete (T*)hte->data;
				htn = hte->next;
				wva_free(hte);
				hte = htn;
			}
		}
		wva_free(this->table);
	}
};

} // namespace Wave

struct wvai_state {
	uint32_t nplatforms;
	uint32_t nls;
};

int wva_allocstate(wvat_state *st)
{
	return 0;
}

int wva_freestate(wvat_state st)
{
	return 0;
}

#define MAXOPCODELEN 255

static void memtolower(char *d, const char *s, size_t l)
{
	size_t x;
	for(x = 0; x < l; x++) d[x] = tolower(s[x]);
}

int wva_loadisf(wvat_state st, char *isftxt, size_t isflen)
{
	size_t x, e;
	int mode;
	const char *line;
	const char *rc;
	size_t ll, ln;
	char opc[MAXOPCODELEN+1];
	x = 0; mode = 0;
	ll = 0;
	ln = 0;
	Wave::arrayhash<std::string> ht;
	Wave::hashentry *hte;
	while(x < isflen) {
		ln++;
		line = isftxt + x;
		for(e = x; e < isflen && isftxt[e] != 10; e++); e;
		ll = e-x;
		if(line[0] == '#') {
			x = e + 1;
			continue;
		}
		switch(mode) {
		case 0:
			if(line[0] == '<') {
				fprintf(stderr, "Line %d - Section: %d-%d\n", ln, x, ll);
			}
			if(strneq(line, ll, "<OPCODE>", 8)) {
				fprintf(stderr, "OPCODE section\n");
				mode = 1;
			}
			break;
		case 1:
			if(strneq(line, ll, "</OPCODE>", 9)) {
				fprintf(stderr, "End OPCODE section\n");
				mode = 0;
				break;
			}
			if(0 != (rc = (const char*)memchr(line, ':', ll))) {
				if(rc - line > MAXOPCODELEN) {
					fprintf(stderr, "ISF: Opcode too long.\n");
					break;
				}
				memtolower(opc, line, rc-line);
				opc[rc-line+1] = 0;
				fprintf(stderr, "ISF: Opcode: %s\n", opc);
				hte = ht.lookup(opc, rc-line);
				if(!hte) {
					fprintf(stderr, "ISF: Adding opcode\n");
					ht.set(opc, rc-line, std::string(rc, ll - (rc-line)));
				} else {
					fprintf(stderr, "ISF: Adding encoding\n");
					std::string &st = *(std::string*)hte->data;
					st.append(rc, ll - (rc-line));
					fprintf(stderr, "ISF: %s\n", st.c_str());
				}
			}
			break;
		default:
			mode = 0;
			break;
		}
		x = e + 1;
	}
	return 0;
}

int wva_loadisfc(wvat_state st, uint8_t *isfdat, size_t isflen)
{
	return 0;
}

int wva_assemble(wvat_state st, wvat_obj *ob, char *atxt, size_t len)
{
	wva_lex(st, atxt, len);
	return 0;
}

int wva_objfree(wvat_obj ob)
{
	return 0;
}

