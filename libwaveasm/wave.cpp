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

#define WAVE_DEBUG 4

#if defined(WAVE_DEBUG) && (WAVE_DEBUG > 0)
#include <stdio.h>
#define WV_DEBUG(a...) fprintf(stderr, a)
#else
#define WV_DEBUG(...)
#endif
#include <string.h>
#include <ctype.h>

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

struct assym {
	size_t start;
	size_t file;
	size_t line;
};

struct assemblyline {
	uint64_t vma;
	std::basic_string<astkn> tkn;
	std::vector<isfopcode> *ops;
	datstring bin;
	assemblyline() {
		vma = 0;
		ops = nullptr;
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
	std::vector<assemblyfile> files;
	wave::arrayhash<assym> symbols;

	assemblystate() {
		cfile = 0;
		cline = 0;
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

static int wva_parse(wvat_state wvst, wave::assemblystate *ast, const char * text, size_t s)
{
	using namespace wave;
	isfstate *isfst = (isfstate*)wvst->isf_state;
	isfdata &st = isfst->isflist[isfst->cisf];
	hashentry<std::vector<isfopcode>> *hopc;
	size_t fn;
	int errc = 0;
	fn = 0;
	assemblyfile *cf;
	if(!ast->files.size()) return 0;
	cf = &ast->files[0];
	// early processing pass
	for(size_t fl = 0; fl < cf->lines.size(); fl++) {
		assemblyline &cl = cf->lines[fl];
		fprintf(stderr, "f%d:l%d:", fn, fl);
		int ulabel = 0;
		int ueval = 0;
		for(int i = 0; i < cl.tkn.size(); i++) {
			astkn &ctkn = cl.tkn[i];
			hashentry<assym> *hte;
			std::string mmm;
			int evchk = 0;
			switch(ctkn.tkn) {
			case TKN_IDENT:
				hte = ast->symbols.lookup(text+ctkn.start, ctkn.len);
				mmm.assign(text+ctkn.start, ctkn.len);
				if(!hte) {
					if(ulabel) {
						fprintf(stderr, "%d:%d: Error: Undefined symbol: %s\n", fn, fl, mmm.c_str());
						errc++;
					}
				} else if(!ueval) {
					fprintf(stderr, "N-IDENT ");
					ueval = 1;
				} else {
					fprintf(stderr, "%d:%d: Error: Syntax: %s\n", fn, fl, mmm.c_str());
					errc++;
				}
				break;
			case TKN_DECNUM:
			case TKN_HEXNUM:
			case TKN_OCTNUM:
			case TKN_BINNUM:
			case TKN_CHAR:
				if(ueval) {
					fprintf(stderr, "NUM ");
				} else {
					fprintf(stderr, "N-NUM ");
					ueval = 1;
				}
				break;
			case TKN_PLUS:
				fprintf(stderr, "P+ ");
				break;
			case TKN_MINUS:
				fprintf(stderr, "P- ");
				break;
			case TKN_STAR:
				fprintf(stderr, "P* ");
				break;
			case TKN_DIV:
				fprintf(stderr, "P/ ");
				break;
			case TKN_LPAREN:
				fprintf(stderr, "P( ");
				ueval = 1;
				break;
			case TKN_RPAREN:
				fprintf(stderr, "PE) ");
				break;
			default:
				if(ueval) {
					fprintf(stderr, "F-NUM ");
					ueval = 0;
				}
				break;
			}
			switch(ctkn.tkn) {
			case TKN_IDENT:
				if(!hte) {
					// make an identifier on the beginning of lines a label
					if(!ulabel) {
						ctkn.tkn = TKN_LABEL;
						ast->symbols.set(text+ctkn.start, ctkn.len, {ctkn.start,fn,fl});
					}
				}
				break;
			case TKN_LABEL:
				hte = ast->symbols.lookup(text+ctkn.start, ctkn.len);
				mmm.assign(text+ctkn.start, ctkn.len);
				if(!hte) {
					ast->symbols.set(text+ctkn.start, ctkn.len, {ctkn.start,fn,fl});
				} else {
					fprintf(stderr, "%d:%d: Error: duplicate symbol: %s\n", fn, fl, mmm.c_str());
					fprintf(stderr, "%d:%d: Info: previous definition here.\n", hte->data->file, hte->data->line);
				}
				break;
			case TKN_DECNUM:
			case TKN_HEXNUM:
			case TKN_OCTNUM:
			case TKN_BINNUM:
			case TKN_CHAR:
			case TKN_PLUS:
			case TKN_MINUS:
			case TKN_STAR:
			case TKN_DIV:
			case TKN_LPAREN:
			case TKN_RPAREN:
				break;
			default:
				fprintf(stderr, "!%02x ", ctkn.tkn);
				break;
			}
			if(ctkn.tkn != 1) {
				ulabel = 1;
			}
		}
		fprintf(stderr, "\n");
	}
	return errc;
}

extern "C"
void lex_add_token(void * state, void *fs, const char * text, size_t s, size_t e, int tk)
{
	using namespace wave;
	wvat_state wvst = (wvat_state)state;
	isfstate *isfst = (isfstate*)wvst->isf_state;
	assemblystate *ast = (assemblystate*)fs;
	assemblyfile &cf = ast->files[ast->cfile];
	assemblyline *cl = nullptr;
	isfdata &st = isfst->isflist[isfst->cisf];
	hashentry<std::vector<isfopcode>> *hto;
	hashentry<isfkey> *htk;
	hashentry<isfreg> *htr;
	while(cf.lines.size() <= ast->cline) {
		cf.lines.push_back(assemblyline());
	}
	cl = &cf.lines[ast->cline];
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
			cl->ops = hto->data;
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
		if(cl->ops) {
			for(size_t x = 0; x < cl->ops->size(); x++) {
				WV_DEBUG(" FMT:\"%s\"", (*cl->ops)[x].fmt.c_str());
			}
			WV_DEBUG("\n");
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
	fst->files.push_back(wave::assemblyfile(atxt, len));
	wva_lex(st, fst, atxt, len);
	wva_parse(st, fst, atxt, len);
	delete fst;
	return 0;
}

int wva_objfree(wvat_obj ob)
{
	return 0;
}

