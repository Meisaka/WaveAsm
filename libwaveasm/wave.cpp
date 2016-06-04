/****
 * WaveAsm wave.c - core functions
 * Copyright (c) 2013-2016, Meisaka Yukara <Meisaka.Yukara at gmail dot com>
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

#define WAVE_DEBUG 4

#if defined(WAVE_DEBUG) && (WAVE_DEBUG > 0)
#include <stdio.h>
#define WV_DEBUG(a...) fprintf(stderr, a)
#else
#define WV_DEBUG(...)
#endif
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include <string>
#include <vector>

#define WVTE_OPCODE 0
#define WVTE_REG 1
#define WVTE_KEYWORD 2
#define WVTE_LIT 3
#define WVTE_GROUP 4

#define WVO_REFCALL 0x20
#define WVO_REF1 0x21
#define WVO_REF2 0x22
#define WVO_REF4 0x23
#define WVO_APPEND(a) (0x40+((a)&0x1f))

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

static void encoderparse(int type, datstring &eout, const std::string &enctxt) {
	size_t el = enctxt.length();
	size_t i;
	int refnum;
	int bitnum = 0;
	uint32_t cstnum = 0;
	uint64_t decnum;
	int mode = 0;
	for(i = 0; i <= el; i++) {
		char cc;
		if(i == el) cc = 0; else cc = enctxt[i];
reparse:
		switch(mode) {
		case 0:
			switch(cc) {
			case 0:
				break;
			case ' ': // control seperator
			case '\t':
				break;
			case '+': // concat operator
				mode = 8;
				break;
			case '\\': // back ref
				mode = 1;
				refnum = 0;
				break;
			case '%': // Bin start code
				mode = 4;
				break;
			case 'L': // length def
				refnum = 0;
				mode = 2;
				break;
			case 'A': // append sequence element
				break;
			case 'P': // prepend sequence element
				break;
			case '0': // Hex/Octal start code
				mode = 5;
				break;
			case '1': // Dec start
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				decnum = cc - '0';
				mode = 3;
				break;
			default:
				WV_DEBUG("Invalid encoder char '%c'\n", cc);
				break;
			}
			switch(mode) {
			case 4:
			case 5:
			case 8:
				break;
			default:
				if(bitnum > 0) {
					while(bitnum > 8) {
						uint8_t v;
						v = cstnum >> (bitnum - 8);
						bitnum -= 8;
						eout.push_back(WVO_APPEND(8));
						eout.push_back(v);
					}
					if(bitnum > 0) {
						uint8_t v;
						v = cstnum & ((1 << bitnum) - 1);
						eout.push_back(WVO_APPEND(bitnum));
						eout.push_back(v);
						bitnum = 0;
					}
				}
				break;
			}
			break;
		case 1: // back ref number
			if(cc >= '0' && cc <= '9') {
				refnum += cc - '0';
			} else {
				if(refnum < 256) {
					eout.push_back(WVO_REF1);
					eout.push_back((uint8_t)refnum);
				} else if(refnum < 65536) {
					eout.push_back(WVO_REF2);
					eout.push_back((uint8_t)(refnum >> 8));
					eout.push_back((uint8_t)(refnum));
					WV_DEBUG("OP APPEND: %02x %04x\n", WVO_REF2, refnum);
				} else {
					eout.push_back(WVO_REF4);
					eout.push_back((uint8_t)(refnum >> 24));
					eout.push_back((uint8_t)(refnum >> 16));
					eout.push_back((uint8_t)(refnum >> 8));
					eout.push_back((uint8_t)(refnum));
					WV_DEBUG("OP APPEND: %02x %08x\n", WVO_REF4, refnum);
				}
				refnum = 0;
				if(cc != '.') {
					eout.push_back(WVO_REFCALL);
					mode = 0; goto reparse;
				}
			}
			break;
		case 2: // length def
			if(cc >= '0' && cc <= '9') {
				refnum *= 10;
				refnum += cc - '0';
			} else {
				WV_DEBUG("ITEM Length def: %d\n", refnum);
				mode = 0; goto reparse;
			}
			break;
		case 3: // Dec
			if(cc >= '0' && cc <= '9') {
				decnum += cc - '0';
			} else {
				WV_DEBUG("ITEM Dec num: %ld\n", decnum);
				mode = 0; goto reparse;
			}
			break;
		case 4: // Bin
			while(bitnum > 8) {
				uint8_t v;
				v = cstnum >> (bitnum - 8);
				bitnum -= 8;
				eout.push_back(WVO_APPEND(8));
				eout.push_back(v);
			}
			if(cc == '0' || cc == '1') {
				cstnum = (cstnum << 1) | (cc - '0');
				bitnum++;
			} else {
				mode = 0; goto reparse;
			}
			break;
		case 5: // Hex/Oct/Zero switch
			switch(cc) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				cstnum = cc - '0';
				bitnum = 3;
				mode = 7;
				break;
			case '8':
			case '9':
				WV_DEBUG("number input error\n");
				mode = 0;
				break;
			case 'x':
			case 'X':
				mode = 6;
				break;
			default:
				WV_DEBUG("ITEM Zero\n");
				mode = 0; goto reparse;
			}
			break;
		case 6: // Hex
			while(bitnum > 8) {
				uint8_t v;
				v = cstnum >> (bitnum - 8);
				bitnum -= 8;
				eout.push_back(WVO_APPEND(8));
				eout.push_back(v);
			}
			if(cc >= '0' && cc <= '9') {
				cstnum = (cstnum << 4) | (cc - '0');
				bitnum += 4;
			} else if(cc >= 'A' && cc <= 'F') {
				cstnum = (cstnum << 4) | (10 + cc - 'A');
				bitnum += 4;
			} else if(cc >= 'a' && cc <= 'f') {
				cstnum = (cstnum << 4) | (10 + cc - 'a');
				bitnum += 4;
			} else {
				mode = 0; goto reparse;
			}
			break;
		case 7: // Oct
			while(bitnum > 8) {
				uint8_t v;
				v = cstnum >> (bitnum - 8);
				bitnum -= 8;
				eout.push_back(WVO_APPEND(8));
				eout.push_back(v);
			}
			if(cc >= '0' && cc <= '7') {
				cstnum = (cstnum << 3) | (cc - '0');
				bitnum += 3;
			} else {
				mode = 0; goto reparse;
			}
			break;
		case 8: // concat
			mode = 0; goto reparse;
			break;
		}
	}
}

struct isfopcode {
	uint32_t arc;
	datstring enc;
	std::string fmt;

	isfopcode(uint32_t argc, const std::string &argf, const std::string &enctxt)
		: arc(argc), fmt(argf)
	{
		encoderparse(WVTE_OPCODE, enc, enctxt);
		WV_DEBUG("Encoding:");
		for(size_t t = 0; t < enc.length(); t++) {
			WV_DEBUG(" %02x", enc[t]);
		}
		WV_DEBUG(" <\n");
	}
};

struct isflit {
	uint64_t ll;
	uint64_t ul;
	uint32_t lv;
	uint32_t ov;
	uint32_t mv;
	std::string fmt;
	datstring enc;
};

struct isfreg {
	std::string fmt;
	datstring enc;

	isfreg(const std::string &rname, const std::string &enctxt) : fmt(rname) {
		encoderparse(WVTE_REG, enc, enctxt);
		WV_DEBUG("Encoding:");
		for(size_t t = 0; t < enc.length(); t++) {
			WV_DEBUG(" %02x", enc[t]);
		}
		WV_DEBUG(" <\n");
	}
};
struct isfkey {
	std::string fmt;
	datstring enc;

	isfkey(const std::string &kname, const std::string &enctxt) : fmt(kname) {
		encoderparse(WVTE_KEYWORD, enc, enctxt);
		WV_DEBUG("Encoding:");
		for(size_t t = 0; t < enc.length(); t++) {
			WV_DEBUG(" %02x", enc[t]);
		}
		WV_DEBUG(" <\n");
	}
};

struct istate {
	uint32_t nplatforms;
	uint32_t nls;
};

struct astkn {
	size_t start;
	size_t len;
	int tkn;
};

struct aspos {
	size_t file;
	size_t line;
	size_t off;
};

struct assym {
	size_t start;
	size_t file;
};

struct assemblyline {
	size_t lstart;
	std::basic_string<astkn> tkn;
	assemblyline(size_t s) {
		lstart = s;
	}
};

struct assemblyfile {
	std::string src;
	std::vector<assemblyline> lines;

	assemblyfile() { }
	assemblyfile(const char *txt, size_t len) : src(txt,len) { }
};

struct assemblystate {
	size_t cfile;
	size_t cline;
	size_t cchar;
	size_t errc;
	size_t llfile;
	std::vector<assemblyfile*> files;
	wave::arrayhash<assym> symbols;

	assemblystate() {
		cfile = 0;
		cline = 0;
		cchar = 0;
		llfile = 0;
		errc = 0;
	}
	~assemblystate() {
		for(size_t t = 0; t < files.size(); t++) {
			delete files[t];
		}
		files.clear();
	}
};

struct isfdata {
	std::string name;
	wave::arrayhash<std::vector<isfopcode>> opht;
	wave::arrayhash<isfreg> reght;
	wave::arrayhash<isfkey> keyht;
	std::vector<isflit> litv;
};

struct isfstate {
	uint32_t cisf;
	std::vector<isfdata> isflist;

	isfstate() {
		cisf = 0;
	}
};

} // namespace wave

enum WVA_PARSE {
	TKN_NONE = 0,
	TKN_WS = 1,
	TKN_HASH = 2,
	TKN_IDENT = 3,
	TKN_DECNUM = 4,
	TKN_BANG = 5,
	TKN_STRING = 6,
	TKN_DS = 7,
	TKN_CENT = 8,
	TKN_AMP = 9,
	TKN_CHAR = 10,
	TKN_LPAREN = 11,
	TKN_RPAREN = 12,
	TKN_STAR = 13,
	TKN_PLUS = 14,
	TKN_COMMA = 15,
	TKN_MINUS = 16,
	TKN_MACRO = 17,
	TKN_DIV = 18,
	TKN_LABEL = 19,
	TKN_SEMIC = 20,
	TKN_LESS = 21,
	TKN_GRTR = 22,
	TKN_EQUL = 23,
	TKN_ELINE = 24,
	TKN_ECOMMENT = 25,
	TKN_HEXNUM = 26,
	TKN_OCTNUM = 27,
	TKN_BINNUM = 28,
	TKN_ATSGN = 29,
	TKN_LBRACK = 30,
	TKN_RBRACK = 31,
	TKN_LBRACE = 32,
	TKN_RBRACE = 33,
	TKN_BKSLSH = 34,
	TKN_OPCODE = 44,
	TKN_KEYWORD = 45,
	TKN_REG = 46,
};

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
	case TKN_WS: c = '4'; break;
	case TKN_IDENT:
		break;
	case TKN_CENT: c = '2'; break;
	case TKN_DECNUM:
	case TKN_HEXNUM:
	case TKN_OCTNUM:
	case TKN_BINNUM: c = '5'; break;
	case TKN_LABEL: c = '3';
		 break;
	case TKN_CHAR:
	case TKN_STRING: c = '1'; break;
	case TKN_OPCODE: c = '1'; b = 1; break;
	case TKN_KEYWORD: c = '2'; b = 1; break;
	case TKN_REG: c = '4'; b = 1; break;
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

struct wva_teval {
	int index;
	int etkn;
	uint64_t val;
};

enum WVA_EVAL_TOKEN {
	ETKN_LPAREN = 2,
	ETKN_RPAREN = 3,
	ETKN_IDENT = 4,
	ETKN_CONST = 5,
	ETKN_ADD = 10,
	ETKN_SUB = 11,
	ETKN_MUL = 12,
	ETKN_DIV = 13,
	ETKN_NEG = 20,
};
#define ETKN_MIN_OP 10
#define ETKN_MAX_OP 13
#define ETKN_MIN_VAL 3
#define ETKN_MAX_VAL 5

#define WV_MACRO_INCLUDE 1

#define DEBUG_PARSE(x...) WV_DEBUG(x)
//#define DEBUG_EVAL(x...) WV_DEBUG(x)
#define DEBUG_EVAL(x...)

static void show_stack(std::vector<wva_teval> &stk) {
	char ft;
	DEBUG_EVAL("S<");
	for(int i = 0; i < stk.size(); i++) {
		switch(stk.at(i).etkn) {
		case ETKN_ADD: ft = '+'; break;
		case ETKN_SUB: ft = '-'; break;
		case ETKN_MUL: ft = '*'; break;
		case ETKN_DIV: ft = '/'; break;
		case ETKN_LPAREN: ft = '('; break;
		case ETKN_NEG: ft = '~'; break;
		case ETKN_RPAREN: ft = ')'; break;
		case ETKN_CONST: ft = 'N'; break;
		case ETKN_IDENT: ft = 'I'; break;
		default:
			ft = '?';
		}
		DEBUG_EVAL("%c", ft);
	}
	DEBUG_EVAL("> ");
}

static int eval_stack(std::vector<wva_teval> &stk)
{
	if(stk.empty()) return 0;
	size_t ss = stk.size();
	switch(stk.back().etkn) {
	case ETKN_ADD:
		if(ss < 3) return -1;
		if(stk.at(ss - 3).etkn == 5 && stk.at(ss - 2).etkn == 5) {
			wva_teval &sl = stk[ss-3];
			wva_teval &sr = stk[ss-2];
			sl.val = sl.val + sr.val;
			if(sr.index > sl.index) sl.index = sr.index;
			stk.pop_back();
			stk.pop_back();
		}
		break;
	case ETKN_SUB:
		if(ss < 3) return -1;
		if(stk.at(ss - 3).etkn == 5 && stk.at(ss - 2).etkn == 5) {
			wva_teval &sl = stk[ss-3];
			wva_teval &sr = stk[ss-2];
			sl.val = sl.val - sr.val;
			if(sr.index > sl.index) sl.index = sr.index;
			stk.pop_back();
			stk.pop_back();
		}
		break;
	case ETKN_MUL:
		if(ss < 3) return -1;
		if(stk.at(ss - 3).etkn == 5 && stk.at(ss - 2).etkn == 5) {
			wva_teval &sl = stk[ss-3];
			wva_teval &sr = stk[ss-2];
			sl.val = sl.val * sr.val;
			if(sr.index > sl.index) sl.index = sr.index;
			stk.pop_back();
			stk.pop_back();
		}
		break;
	case ETKN_DIV:
		if(ss < 3) return -1;
		if(stk.at(ss - 3).etkn == 5 && stk.at(ss - 2).etkn == 5) {
			wva_teval &sl = stk[ss-3];
			wva_teval &sr = stk[ss-2];
			if(!sr.val) return -1;
			sl.val = sl.val / sr.val;
			if(sr.index > sl.index) sl.index = sr.index;
			stk.pop_back();
			stk.pop_back();
		}
		break;
	}
	return 0;
}

static int wva_error(wave::assemblystate *ast, const char *argf, const char *msgfmt, ...)
{
	va_list va;
	ast->errc++;
	va_start(va, msgfmt);
	wave::assemblyline &cl = ast->files[ast->cfile]->lines[ast->cline];
	size_t ls = cl.lstart;
	size_t ll = 0;
	size_t lc = ast->cchar - ls;
	if(cl.tkn.size() > 0) {
		ll = cl.tkn.back().start - ls;
	}
	fprintf(stderr, "\n%d:%ld:*: %s\n", ast->cfile, ast->cline+1, ast->files[ast->cfile]->src.substr(ls, ll).c_str());
	fprintf(stderr, "%d:%ld:*: ", ast->cfile, ast->cline+1);
	for(size_t z = 0; z < lc; z++) fputc(' ', stderr);
	fputc('^', stderr); fputc(10, stderr);
	fprintf(stderr, "%d:%ld:%ld: Error: ", ast->cfile, ast->cline+1, lc);
	vfprintf(stderr, msgfmt, va);
	fputc(10, stderr);
	va_end(va);
	return 0;
}

static int wva_loadfile(wvat_state wvst, wave::assemblystate *ast, const char * fname, int search)
{
	size_t svfn = ast->cfile;
	size_t svfl = ast->cline;
	size_t size = 0;
	char *data;
	if(wvst->onload) {
		if(wvst->onload(fname, search, &data, &size, wvst->onload_usr)) {
			wva_error(ast, "%s", "failed to load file \"%s\"", fname);
			return -1;
		}
		ast->cline = 0;
		ast->cfile = ast->files.size();
		ast->llfile = ast->cfile;
		ast->files.push_back(new wave::assemblyfile(data, size));
		wva_lex(wvst, ast, data, size);
		wva_free(data);
	} else {
		wva_error(ast, "%s", "Can not load file \"%s\" - no load function defined", fname);
		return -1;
	}
	ast->cfile = svfn;
	ast->cline = svfl;
	return 0;
}

static int wva_parse(wvat_state wvst, wave::assemblystate *ast)
{
	using namespace wave;
	isfstate *isfst = (isfstate*)wvst->isf_state;
	isfdata &st = isfst->isflist[isfst->cisf];
	hashentry<std::vector<isfopcode>> *hopc;
	ast->cfile = 0;
	ast->errc = 0;
	assemblyfile *cf;
	if(!ast->files.size()) return 0;
	bool loadfile = false;
	std::vector<aspos> filestack;
	cf = ast->files[ast->cfile];
	// early processing pass
	for(size_t fl = 0; loadfile || filestack.size() || fl < cf->lines.size(); fl++) {
		if(loadfile) {
			filestack.push_back({ast->cfile, fl, 0});
			ast->cfile = ast->llfile;
			fl = 0;
			cf = ast->files[ast->cfile];
			loadfile = 0;
		}
		if(filestack.size() && !(fl < cf->lines.size())) {
			ast->cfile = filestack.back().file;
			fl = ast->cline = filestack.back().line;
			cf = ast->files[ast->cfile];
			filestack.pop_back();
		}
		assemblyline &cl = cf->lines[fl];
		const char *text = cf->src.data();
		ast->cline = fl;
		DEBUG_PARSE("f%d:l%d:", ast->cfile, ast->cline+1);
		int ulabel = 0;
		uint64_t rval;
		std::vector<wva_teval> estack;
		std::vector<wva_teval> cstack;
		int letkn = 0;
		int macro = 0;
		for(int i = 0; i < cl.tkn.size(); i++) {
			astkn &ctkn = cl.tkn[i];
			ast->cchar = ctkn.start;
			hashentry<assym> *hte;
			std::string mmm;
			/* Eval parsing */
			int etype = 0;
			switch(ctkn.tkn) {
			case TKN_WS:
				break;
			case TKN_IDENT:
				hte = ast->symbols.lookup(text+ctkn.start, ctkn.len);
				mmm.assign(text+ctkn.start, ctkn.len);
				if(!hte) {
					if(ulabel) {
						//wva_error(ast, "%s", "Undefined symbol: %s", mmm.c_str());
					}
				} else {
					DEBUG_PARSE("IDENT ");
					estack.push_back({i,ETKN_IDENT,0});
					letkn = ETKN_IDENT;
					// show_stack(estack); DEBUG_EVAL("\n");
				}
				break;
			case TKN_DECNUM:
				rval = 0;
				for(int x = 0; x < ctkn.len; x++) {
					ast->cchar = ctkn.start+x;
					char ctxt = *(text+ast->cchar);
					rval *= 10;
					if(ctxt >= '0' && ctxt <= '9') rval += ctxt - '0';
					else wva_error(ast, 0, "DECNUM-ERR-INVALID-CHAR:'%c'", ctxt);
				}
				DEBUG_EVAL("DECNUMv=%lu ", rval);
				estack.push_back({i,ETKN_CONST,rval});
				if(!cstack.empty() && cstack.back().etkn == ETKN_NEG) {
					wva_teval &inv = estack.back();
					inv.val = (~inv.val) + 1;
					cstack.pop_back();
				}
				letkn = ETKN_CONST;
				break;
			case TKN_HEXNUM:
				rval = 0;
				for(int x = 2; x < ctkn.len; x++) {
					ast->cchar = ctkn.start+x;
					char ctxt = *(text+ast->cchar);
					rval *= 16;
					if(ctxt >= '0' && ctxt <= '9') rval += ctxt - '0';
					else if(ctxt >= 'A' && ctxt <= 'F') rval += 10 + (ctxt - 'A');
					else if(ctxt >= 'a' && ctxt <= 'f') rval += 10 + (ctxt - 'A');
					else wva_error(ast, 0, "HEXNUM-ERR-INVALID-CHAR");
				}
				DEBUG_EVAL("HEXNUMv=%lu ", rval);
				estack.push_back({i,ETKN_CONST,rval});
				if(!cstack.empty() && cstack.back().etkn == ETKN_NEG) {
					wva_teval &inv = estack.back();
					inv.val = (~inv.val) + 1;
					cstack.pop_back();
				}
				letkn = ETKN_CONST;
				break;
			case TKN_OCTNUM:
				rval = 0;
				for(int x = 1; x < ctkn.len; x++) {
					ast->cchar = ctkn.start+x;
					char ctxt = *(text+ast->cchar);
					rval *= 8;
					if(ctxt >= '0' && ctxt <= '7') rval += ctxt - '0';
					else wva_error(ast, 0, "OCTNUM-ERR-INVALID-CHAR");
				}
				DEBUG_EVAL("OCTNUMv=%lu ", rval);
				estack.push_back({i,ETKN_CONST,rval});
				if(!cstack.empty() && cstack.back().etkn == ETKN_NEG) {
					wva_teval &inv = estack.back();
					inv.val = (~inv.val) + 1;
					cstack.pop_back();
				}
				letkn = ETKN_CONST;
				break;
			case TKN_BINNUM:
				rval = 0;
				for(int x = 2; x < ctkn.len; x++) {
					ast->cchar = ctkn.start+x;
					char ctxt = *(text+ast->cchar);
					rval *= 2;
					if(ctxt >= '0' && ctxt <= '1') rval += ctxt - '0';
					else wva_error(ast, 0, "BINNUM-ERR-INVALID-CHAR");
				}
				DEBUG_EVAL("BINNUMv=%lu ", rval);
				estack.push_back({i,ETKN_CONST,rval});
				if(!cstack.empty() && cstack.back().etkn == ETKN_NEG) {
					wva_teval &inv = estack.back();
					inv.val = (~inv.val) + 1;
					cstack.pop_back();
				}
				letkn = ETKN_CONST;
				break;
			case TKN_CHAR:
				{
					uint8_t ctxt = (uint8_t)*(text+ctkn.start+1);
					if(ctxt == '\\') {
						ctxt = (uint8_t)*(text+ctkn.start+2);
					}
					DEBUG_EVAL("CHARNUMv=%u ", ctxt);
					estack.push_back({i,ETKN_CONST,ctxt});
				}
				if(!cstack.empty() && cstack.back().etkn == ETKN_NEG) {
					wva_teval &inv = estack.back();
					inv.val = (~inv.val) + 1;
					cstack.pop_back();
				}
				letkn = ETKN_CONST;
				break;
			case TKN_PLUS:
				if(!etype) {
					if(!letkn) break;
					etype = ETKN_ADD; DEBUG_EVAL("P+ ");
				}
			case TKN_MINUS:
				if(!etype) {
					etype = ETKN_SUB;
					// Anything where LHS isn't a value makes this a negate
					if(letkn < ETKN_MIN_VAL || letkn > ETKN_MAX_VAL) {
						etype = ETKN_NEG;
					}
					DEBUG_EVAL("P- ");
				}
			case TKN_STAR:
				if(!etype) {
					if(!letkn) break;
					etype = ETKN_MUL; DEBUG_EVAL("P* ");
				}
			case TKN_DIV:
				if(!etype) {
					if(!letkn) break;
					etype = ETKN_DIV; DEBUG_EVAL("P/ ");
				}
				if(!cstack.empty()) {
					while(!cstack.empty() && cstack.back().etkn >= etype) {
						DEBUG_EVAL("NPOP ");
						estack.push_back(cstack.back());
						cstack.pop_back();
						if(eval_stack(estack) < 0) {
							wva_error(ast, 0, "EVAL-IMM-ERR");
						}
					}
				}
				cstack.push_back({i,etype,0});
				letkn = etype;
				//show_stack(estack); show_stack(cstack); DEBUG_EVAL("\n");
				break;
			case TKN_LPAREN:
				DEBUG_EVAL("P( ");
				cstack.push_back({i,ETKN_LPAREN,0});
				//show_stack(estack); DEBUG_EVAL("\n");
				letkn = ETKN_LPAREN;
				break;
			case TKN_RPAREN:
				if(!letkn) break;
				DEBUG_EVAL("E) ");
				while(!cstack.empty()) {
					DEBUG_EVAL("NPOP ");
					if(cstack.back().etkn == ETKN_LPAREN) {
						cstack.pop_back();
						etype = ETKN_RPAREN;
						break;
					}
					estack.push_back(cstack.back());
					cstack.pop_back();
					if(eval_stack(estack) < 0) {
						wva_error(ast, 0, "EVAL-IMM-ERR");
					}
				}
				if(!etype) wva_error(ast, 0, "RPAREN ERROR!");
				show_stack(estack); show_stack(cstack);
				letkn = ETKN_RPAREN;
				break;
			default:
				if(!cstack.empty()) {
					while(!cstack.empty()) {
						DEBUG_EVAL("NPOP ");
						if(cstack.back().etkn == ETKN_LPAREN) wva_error(ast, 0, "LPAREN ERROR!");
						estack.push_back(cstack.back());
						cstack.pop_back();
						if(eval_stack(estack) < 0) {
							wva_error(ast, 0, "EVAL-IMM-ERR");
						}
					}
					show_stack(estack); show_stack(cstack);
				}
				if(!estack.empty()) {
					DEBUG_PARSE("F-NUM ");
					show_stack(estack);
					int nnums = 0;
					int nops = 0;
					int e;
					for(int i = 0; i < estack.size(); i++) {
						e = estack.at(i).etkn;
						DEBUG_EVAL("E%d.", e);
						if(e >= ETKN_MIN_OP && e <= ETKN_MAX_OP) {
							nops++;
						} else if(e >= ETKN_MIN_VAL && e <= ETKN_MAX_VAL) {
							nnums++;
							DEBUG_EVAL("v=%lx ", estack.at(i).val);
						} else {
							wva_error(ast, 0, "EXPR CONTENT ERROR!");
						}
					}
					if(nops + 1 != nnums) {
						wva_error(ast, 0, "EXPR SYNTAX ERROR!");
					}
					DEBUG_EVAL("\n");
					estack.clear();
					cstack.clear();
				}
				letkn = 0;
				break;
			}
			/* Resolve parsing */
			switch(ctkn.tkn) {
			case TKN_WS:
				if(!i && ctkn.len > 11 && 0 == strncmp(text+ctkn.start, "#include ", 9)) {
					int ince = 0;
					int incs = text[ctkn.start+9];
					int si = 10;
					if(incs == '<') ince = '>';
					else if(incs == '"') ince = '"';
					else if(incs == '\'') ince = '\'';
					else { ince = ' '; si = 9; }
					int i = si;
					while(i < ctkn.len && text[ctkn.start+i] != ince) {
						i++;
					}
					std::string ifile(text+ctkn.start+si, i-si);
					DEBUG_PARSE("FOUND INCLUDE [%s]", ifile.c_str());
					if(wva_loadfile(wvst, ast, ifile.c_str(), incs == '<'? 1:0)) {
						return -1;
					}
					loadfile = true;
				}
				break;
			case TKN_ELINE:
				DEBUG_PARSE("EOL");
				break;
			case TKN_IDENT:
				if(!hte) {
					// make an identifier on the beginning of lines a label
					if(!ulabel) {
						ctkn.tkn = TKN_LABEL;
						ast->symbols.set(text+ctkn.start, ctkn.len, {ctkn.start,ast->cfile});
						DEBUG_PARSE("SLABEL ");
					}
				}
				break;
			case TKN_LABEL:
				hte = ast->symbols.lookup(text+ctkn.start, ctkn.len);
				mmm.assign(text+ctkn.start, ctkn.len);
				if(!hte) {
					ast->symbols.set(text+ctkn.start, ctkn.len, {ctkn.start,ast->cfile});
					DEBUG_PARSE("MLABEL ");
				} else {
					wva_error(ast, "%s", "duplicate symbol: %s", mmm.c_str());
					//wva_error(hte->data->file, hte->data->line, "previous definition here.");
				}
				break;
			case TKN_STRING:
				if(!macro) {
					wva_error(ast, 0, "Unexpected string");
					break;
				}
				if(macro == WV_MACRO_INCLUDE) {
					std::string ifile(text+ctkn.start + 1, ctkn.len - 2);
					DEBUG_PARSE("MINCLUDE [%s] ", ifile.c_str());
					if(wva_loadfile(wvst, ast, ifile.c_str(), 0)) {
						return -1;
					}
					loadfile = true;
				}
				break;
			case TKN_MACRO:
				if(ctkn.len == 8 && 0 == strncmp(text+ctkn.start, ".include", 8)) {
					macro = WV_MACRO_INCLUDE;
				} else {
					macro = -1;
				}
				break;
			case TKN_COMMA:
				DEBUG_PARSE("COMMA ");
				break;
			case TKN_DECNUM:
			case TKN_HEXNUM:
			case TKN_OCTNUM:
			case TKN_BINNUM:
			case TKN_CHAR:
				break;
			case TKN_PLUS:
				if(letkn) break;
				DEBUG_PARSE("SYM+ ");
				break;
			case TKN_MINUS:
				if(letkn) break;
				DEBUG_PARSE("SYM- ");
				break;
			case TKN_STAR:
				if(letkn) break;
				DEBUG_PARSE("SYM* ");
				break;
			case TKN_DIV:
				if(letkn) break;
				DEBUG_PARSE("SYM/ ");
				break;
			case TKN_LPAREN:
				if(letkn) break;
				DEBUG_PARSE("SYM( ");
				break;
			case TKN_RPAREN:
				if(letkn) break;
				DEBUG_PARSE("SYM) ");
				break;
			case TKN_LBRACK:
				DEBUG_PARSE("SYM[ ");
				break;
			case TKN_RBRACK:
				DEBUG_PARSE("SYM] ");
				break;
			case TKN_KEYWORD:
				DEBUG_PARSE("KEYW ");
				break;
			case TKN_REG:
				DEBUG_PARSE("REG ");
				break;
			case TKN_OPCODE:
				DEBUG_PARSE("OPC ");
				break;
			default:
				DEBUG_PARSE("!%02x ", ctkn.tkn);
				break;
			}
			if(ctkn.tkn != 1) {
				ulabel = 1;
			}
		}
		DEBUG_PARSE("\n");
	}
	return ast->errc;
}

extern "C"
void lex_add_token(void * state, void *fs, const char * text, size_t s, size_t e, int tk)
{
	using namespace wave;
	wvat_state wvst = (wvat_state)state;
	isfstate *isfst = (isfstate*)wvst->isf_state;
	assemblystate *ast = (assemblystate*)fs;
	assemblyfile *cf = ast->files[ast->cfile];
	assemblyline *cl = nullptr;
	isfdata &st = isfst->isflist[isfst->cisf];
	hashentry<std::vector<isfopcode>> *hto;
	hashentry<isfkey> *htk;
	hashentry<isfreg> *htr;
	while(cf->lines.size() <= ast->cline) {
		cf->lines.push_back(assemblyline(s));
	}
	cl = &cf->lines[ast->cline];
	if(tk == TKN_LABEL) {
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
	case TKN_IDENT:
	case TKN_CENT:
	case TKN_MACRO:
	case TKN_LABEL:
		if((hto = st.opht.lookup(mmm.data(), mmm.length()))) {
			tk = TKN_OPCODE;
		} else if((htr = st.reght.lookup(mmm.data(), mmm.length()))) {
			tk = TKN_REG;
		} else if((htk = st.keyht.lookup(mmm.data(), mmm.length()))) {
			tk = TKN_KEYWORD;
		}
		diag_color(mmm.c_str(),0,e-s,tk);
		break;
	case TKN_ELINE:
		ast->cline++;
		fprintf(stderr,"\n");
		break;
	default:
		diag_color(text,s,e,tk);
		break;
	}
	cl->tkn.push_back({s,e-s,tk});
	if(tk == TKN_ELINE) {
		for(int i = 0; i < cl->tkn.size(); i++) {
		}
	}
}

int wva_loadisf(wvat_state st, char *isftxt, size_t isflen)
{
	using namespace wave;
	size_t x, e;
	int mode;
	const char *line;
	const char *rc;
	const char *ic;
	size_t ll, ln;
	std::string opc;
	wave::isfstate &isf = *(wave::isfstate*)st->isf_state;
	x = 0; mode = 0;
	ll = 0;
	ln = 0;
	isf.isflist.resize(isf.isflist.size()+1);
	wave::isfdata &cisf = isf.isflist[isf.isflist.size()-1];
	std::string secname;
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
				WV_DEBUG("Line %d - Section: %d-%d\n", ln, x, ll);
			}
			if(strneq(line, ll, "<OPCODE>", 8)) {
				WV_DEBUG("OPCODE section\n");
				mode = 1;
			} else if(strneq(line, ll, "<HEAD>", 6)) {
				WV_DEBUG("HEAD section\n");
				mode = 2;
			} else if(strneq(line, ll, "<REG>", 5)) {
				WV_DEBUG("REG section\n");
				mode = 3;
			} else if(strneq(line, ll, "<LIT>", 5)) {
				WV_DEBUG("LIT section\n");
				mode = 4;
			} else if(strneq(line, ll, "<KEYWORD>", 9)) {
				WV_DEBUG("KEYWORD section\n");
				mode = 5;
			} else if(strneq(line, ll, "<GROUP>", 7)) {
				WV_DEBUG("GROUP section\n");
				mode = 6;
			}
			break;
		case 1:
			if(strneq(line, ll, "</OPCODE>", 9)) {
				WV_DEBUG("End OPCODE section\n");
				mode = 0;
				break;
			}
			if(0 != (rc = (const char*)memchr(line, ':', ll))) {
				stringlower(opc, line, rc-line);
				wave::hashentry<std::vector<wave::isfopcode>> *hte;
				hte = cisf.opht.lookup(opc.data(), opc.length());
				rc++;
				std::string linedat(rc, ll - (rc-line));
				size_t n = linedat.find_first_of(':', 0);
				std::string lst(linedat.substr(0, n));
				size_t k = linedat.find_first_of(':', ++n);
				std::string fst(linedat.substr(n, k-n));
				std::string est(linedat.substr(1+k));
				uint32_t ol = 0;
				isfopcode io(ol, fst, est);
				if(!hte) {
					WV_DEBUG("ISF: Adding opcode %s - %s - %s - %s\n", opc.c_str(), lst.c_str(), fst.c_str(), est.c_str());
					std::vector<isfopcode> iov;
					iov.push_back(io);
					cisf.opht.set(opc.data(), opc.length(), iov);
				} else {
					std::vector<isfopcode> &st = *hte->data;
					WV_DEBUG("ISF: Extending opcode %s - %s - %s - %s\n", opc.c_str(), lst.c_str(), fst.c_str(), est.c_str());
					st.push_back(io);
				}
			}
			break;
		case 2:
			if(strneq(line, ll, "</HEAD>", 7)) {
				WV_DEBUG("End HEAD section\n");
				mode = 0;
				break;
			}
			if(0 != (rc = (const char*)memchr(line, ':', ll))) {
				std::string item(line, rc-line);
				WV_DEBUG("ISF: %s\n", item.c_str());
			}
			break;
		case 3:
			if(strneq(line, ll, "</REG>", 6)) {
				WV_DEBUG("End REG section\n");
				mode = 0;
				break;
			}
			if(line[0] == '+') {
			} else if(0 != (rc = (const char*)memchr(line, ':', ll))) {
				std::string enct(rc+1, ll - (rc-line) - 1);
				ic = (const char*)memchr(line, ',', rc-line);
				if(!ic) ic = rc;
				do {
					std::string item;
					stringlower(item, line, ic-line);
					hashentry<isfreg> *hte;
					hte = cisf.reght.lookup(item.data(), item.length());
					WV_DEBUG("ISF: %s :\"%s\"\n", item.c_str(), enct.c_str());
					isfreg io(secname, enct);
					if(!hte) {
						cisf.reght.set(item.data(), item.length(), io);
					}
					line = ic + 1;
					ic = (const char*)memchr(line, ',', rc-line);
					if(!ic) ic = rc;
				} while(line < rc);
			} else {
				std::string item;
				std::string enct;
				stringlower(item, line, ll);
				if(item.length() < 1) break;
				hashentry<isfreg> *hte;
				hte = cisf.reght.lookup(item.data(), item.length());
				isfreg io(secname, enct);
				if(!hte) {
					cisf.reght.set(item.data(), item.length(), io);
				}
			}
			break;
		case 4:
			if(strneq(line, ll, "</LIT>", 6)) {
				WV_DEBUG("End LIT section\n");
				mode = 0;
				break;
			}
			if(0 != (rc = (const char*)memchr(line, ':', ll))) {
				std::string item(line, rc-line);
				std::string enct(rc+1, ll - (rc-line) - 1);
				WV_DEBUG("ISF: %s:%s\n", item.c_str(), enct.c_str());
			}
			break;
		case 5:
			if(strneq(line, ll, "</KEYWORD>", 10)) {
				WV_DEBUG("End KEYWORD section\n");
				mode = 0;
				break;
			}
			if(line[0] == '+') {
			} else if(0 != (rc = (const char*)memchr(line, ':', ll))) {
				std::string enct(rc+1, ll - (rc-line) - 1);
				std::string item;
				stringlower(item, line, rc-line);
				hashentry<isfkey> *hte;
				hte = cisf.keyht.lookup(item.data(), item.length());
				WV_DEBUG("ISF: %s :\"%s\"\n", item.c_str(), enct.c_str());
				isfkey io(secname, enct);
				if(!hte) {
					cisf.keyht.set(item.data(), item.length(), io);
				}
			} else {
				std::string item;
				std::string enct;
				stringlower(item, line, ll);
				if(item.length() < 1) break;
				hashentry<isfkey> *hte;
				hte = cisf.keyht.lookup(item.data(), item.length());
				isfkey io(secname, enct);
				if(!hte) {
					cisf.keyht.set(item.data(), item.length(), io);
				}
			}
			break;
		case 6:
			if(strneq(line, ll, "</GROUP>", 8)) {
				WV_DEBUG("End GROUP section\n");
				mode = 0;
				break;
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
	wave::assemblystate *fst = new wave::assemblystate();
	fst->files.push_back(new wave::assemblyfile(atxt, len));
	wva_lex(st, fst, atxt, len);
	wva_parse(st, fst);
	delete fst;
	return 0;
}

int wva_objfree(wvat_obj ob)
{
	return 0;
}

