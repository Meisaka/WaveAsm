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
#include <vector>

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

namespace wave {

template<typename T>
struct hashentry {
	uint32_t keylen;
	char *key;
	uint32_t datalen;
	T *data;
	hashentry<T> * next;
};

template<typename T>
hashentry<T>* wva_getentryhash(hashentry<T>* hte, const char * key, uint32_t kl) {
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
	hashentry<T> ** table;
public:
	hashentry<T>* lookup(const char *key, uint32_t kl) { 
		uint32_t h;

		if(!this) return 0;
		h = wva_murmur3(key, kl) % this->tablesize;
		return wva_getentryhash(this->table[h], key, kl);
	}
	int set(const char * key, uint32_t kl, const T &d) {
		uint32_t h;
		uint8_t *td;
		hashentry<T>* hte;
		hashentry<T>* htn;

		if(!this || !key || !kl) return 0;
		h = wva_murmur3(key, kl) % this->tablesize;
		hte = this->table[h];
		if(!hte) {
			hte = (hashentry<T>*)wva_alloc(sizeof(hashentry<T>));
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
				hte = (hashentry<T>*)wva_alloc(sizeof(hashentry<T>));
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
		this->table = (hashentry<T>**)arr;
	}
	~arrayhash() {
		if(!this || !this->table) return;
		int i;
		hashentry<T>* hte;
		hashentry<T>* htn;
		for(i = 0; i < this->tablesize; i++) {
			hte = this->table[i];
			while(hte) {
				if(hte->key) wva_free(hte->key);
				if(hte->data) delete hte->data;
				htn = hte->next;
				wva_free(hte);
				hte = htn;
			}
		}
		wva_free(this->table);
	}
};

typedef std::basic_string<uint8_t> datstring;

struct isfopcode {
	uint32_t arc;
	datstring enc;
	std::string arf;
	std::string encode;

	isfopcode(uint32_t argc, const std::string &argf, const std::string &enctxt)
		: arc(argc), arf(argf), encode(enctxt)
	{
	}
};

struct istate {
	uint32_t nplatforms;
	uint32_t nls;
};

struct isfdata {
	std::string name;
	wave::arrayhash<std::vector<isfopcode>> opht;
};

struct isfstate {
	uint32_t cisf;
	std::vector<isfdata> isflist;

	isfstate() {
		cisf = 0;
	}
};

} // namespace wave

int wva_allocstate(wvat_state *est)
{
	wvat_state st;
	st = new wvas_state();
	st->in_state = new wave::istate();
	st->isf_state = new wave::isfstate();

	*est = st;
	return 0;
}

int wva_freestate(wvat_state st)
{
	return 0;
}

static void memtolower(char *d, const char *s, size_t l)
{
	size_t x;
	for(x = 0; x < l; x++) d[x] = tolower(s[x]);
}

static void stringlower(std::string &d, const char *s, size_t l)
{
	size_t x;
	d.resize(l);
	for(x = 0; x < l; x++) d[x] = tolower(s[x]);
}

static void diag_color(const char * txt, size_t s, size_t e, int tk) {
	int c = 0;
	int b = 0;
	switch(tk) {
	case 1: c = '4'; break;
	case 3:
		fprintf(stderr, "H$%08x ", wva_murmur3(txt+s,e-s));
		break;
	case 8: c = '2'; break;
	case 4:
	case 27:
	case 26:
	case 28: c = '5'; break;
	case 19: c = '3';
		 fprintf(stderr, "H$%08x ", wva_murmur3(txt+s,e-s));
		 break;
	case 10:
	case 6: c = '1'; break;
	case 44: c = '1'; b = 1; break;
	default: b = 1; c = '7'; break;
	}
	if(b) fprintf(stderr, "\e[1m");
	if(c) fprintf(stderr, "\e[3%cm", c);
	size_t x;
	for(x = s; x < e; x++) {
		fputc(txt[x], stderr);
	}
	fprintf(stderr, "\e[0m");
}

extern "C"
void lex_add_token(void * state, const char * text, size_t s, size_t e, int tk)
{
	wave::isfdata &st = *(wave::isfdata*)state;
	wave::hashentry<std::vector<wave::isfopcode>> *hte;
	if(tk == 19) {
		if(text[s] == ':') {
			s++;
		} else if(text[e-1] == ':') {
			e--;
		}
	}
	if(s > e) {
		fprintf(stderr, "Index out of bounds s=%d, e=%d\n",s,e);
	}
	std::string mmm;
	stringlower(mmm, text+s, e-s);
	switch(tk) {
	case 3:
	case 19:
		hte = st.opht.lookup(mmm.data(), mmm.length());
		if(hte) tk = 44;
		diag_color(mmm.c_str(),0,e-s,tk);
		break;
	default:
		diag_color(text,s,e,tk);
		break;
	}
}

int wva_loadisf(wvat_state st, char *isftxt, size_t isflen)
{
	using wave::isfopcode;
	size_t x, e;
	int mode;
	const char *line;
	const char *rc;
	size_t ll, ln;
	std::string opc;
	wave::isfstate &isf = *(wave::isfstate*)st->isf_state;
	x = 0; mode = 0;
	ll = 0;
	ln = 0;
	isf.isflist.resize(isf.isflist.size()+1);
	wave::isfdata &cisf = isf.isflist[isf.isflist.size()-1];
	wave::hashentry<std::vector<wave::isfopcode>> *hte;
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
			} else if(strneq(line, ll, "<HEAD>", 6)) {
				fprintf(stderr, "HEAD section\n");
				mode = 2;
			} else if(strneq(line, ll, "<REG>", 5)) {
				fprintf(stderr, "REG section\n");
				mode = 3;
			} else if(strneq(line, ll, "<LIT>", 5)) {
				fprintf(stderr, "LIT section\n");
				mode = 4;
			}
			break;
		case 1:
			if(strneq(line, ll, "</OPCODE>", 9)) {
				fprintf(stderr, "End OPCODE section\n");
				mode = 0;
				break;
			}
			if(0 != (rc = (const char*)memchr(line, ':', ll))) {
				stringlower(opc, line, rc-line);
				hte = cisf.opht.lookup(opc.data(), opc.length());
				rc++;
				std::string linedat(rc, ll - (rc-line));
				size_t n = linedat.find_first_of(':', 0);
				std::string lst(linedat.substr(0, n));
				size_t k = linedat.find_first_of(':', ++n);
				std::string fst(linedat.substr(n, k-n));
				std::string est(linedat.substr(1+k));
				uint32_t ol = 0;
				isfopcode io(ol, fst, linedat);
				if(!hte) {
					fprintf(stderr, "ISF: Adding opcode %s - %s - %s - %s\n", opc.c_str(), lst.c_str(), fst.c_str(), est.c_str());
					std::vector<isfopcode> iov;
					iov.push_back(io);
					cisf.opht.set(opc.data(), opc.length(), iov);
				} else {
					std::vector<isfopcode> &st = *hte->data;
					fprintf(stderr, "ISF: Extending opcode %s - %s - %s - %s\n", opc.c_str(), lst.c_str(), fst.c_str(), est.c_str());
					st.push_back(io);
				}
			}
			break;
		case 2:
			if(strneq(line, ll, "</HEAD>", 7)) {
				fprintf(stderr, "End HEAD section\n");
				mode = 0;
				break;
			}
			if(0 != (rc = (const char*)memchr(line, ':', ll))) {
				std::string item(line, rc-line);
				fprintf(stderr, "ISF: %s\n", item.c_str());
			}
			break;
		case 3:
			if(strneq(line, ll, "</REG>", 6)) {
				fprintf(stderr, "End REG section\n");
				mode = 0;
				break;
			}
			if(0 != (rc = (const char*)memchr(line, ':', ll))) {
				std::string item(line, rc-line);
				fprintf(stderr, "ISF: %s\n", item.c_str());
			}
			break;
		case 4:
			if(strneq(line, ll, "</LIT>", 6)) {
				fprintf(stderr, "End LIT section\n");
				mode = 0;
				break;
			}
			if(0 != (rc = (const char*)memchr(line, ':', ll))) {
				std::string item(line, rc-line);
				fprintf(stderr, "ISF: %s\n", item.c_str());
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
	wave::isfstate &isf = *(wave::isfstate*)st->isf_state;
	if(isf.isflist.size() == 0) {
		fprintf(stderr, "WAVEASM: No ISF loaded\n");
		return 1;
	}
	wva_lex(&isf.isflist[0], atxt, len);
	return 0;
}

int wva_objfree(wvat_obj ob)
{
	return 0;
}

