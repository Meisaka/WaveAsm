/****
 * WaveAsm lexer.c - The Lexer
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
#include "wave.h"
#include "wavefunc.h"

#include <stdio.h>

//#define LEXDEBUG(a) a
#define LEXDEBUG(a)

#define MAKETOKEN(a) fprintf(stderr, "Tkn-" #a)

#define TKWS 1
#define TKHASH 2
#define TKL 3
#define TKDEC 4
#define TKBANG 5
#define TKDQ 6
#define TKDS 7
#define TKPCS 8
#define TKAMP 9
#define TKSQ 10
#define TKLP 11
#define TKRP 12
#define TKSTAR 13
#define TKPLUS 14
#define TKCMA 15
#define TKMINUS 16
#define TKDOT 17
#define TKFSL 18
#define TKCOL 19
#define TKSCOL 20
#define TKLT 21
#define TKGT 22
#define TKEQ 23
#define TKEOL 24
#define TKCOMMENT 25
#define TKHEX 26
#define TKOCT 27
#define TKBIN 28
#define TKAT 29
#define TKLBRK 30
#define TKRBRK 31
#define TKLBRC 32
#define TKRBRC 33
#define TKBSL 34

// these tables determine which mode to switch to based on charactor input.
static int sttab[][256] = {
{
	0,0,0,0,0,0,0,0,0,0,TKEOL,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	TKWS,TKBANG,TKDQ,TKHASH,TKDS,TKPCS,TKAMP,TKSQ,
	TKLP,TKRP,TKSTAR,TKPLUS,TKCMA,TKMINUS,TKDOT,TKFSL,
	26,4,4,4,4,4,4,4,4,4,TKCOL,TKCOMMENT,TKLT,TKEQ,TKGT,0,
	33,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,37,44,38,0,3,
	0,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,39,0,40,0,0,
},
{
	0,0,0,0,0,0,0,0,0,0,TKEOL,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	TKWS,TKBANG,TKDQ,TKHASH,TKDS,TKPCS,TKAMP,TKSQ,
	TKLP,TKRP,TKSTAR,TKPLUS,TKCMA,TKMINUS,TKDOT,TKFSL,
	31,31,31,31,31,31,31,31,0,0,TKCOL,TKCOMMENT,TKLT,TKEQ,TKGT,0,
	33,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,27,0,0,0,0,0,0,0,
	0,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,27,0,0,0,0,0,0,0,
},
{
	0,0,0,0,0,0,0,0,0,0,TKEOL,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	TKWS,TKBANG,TKDQ,TKHASH,TKDS,TKPCS,TKAMP,TKSQ,
	TKLP,TKRP,TKSTAR,TKPLUS,TKCMA,TKMINUS,28,TKFSL,
	28,28,28,28,28,28,28,28,28,28,29,TKCOMMENT,TKLT,TKEQ,TKGT,0,
	33,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,
	28,28,28,28,28,28,28,28,28,28,28,37,44,38,0,28,
	0,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,
	28,28,28,28,28,28,28,28,28,28,28,39,0,40,0,0,
},
{
	0,0,0,0,0,0,0,0,0,0,TKEOL,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	TKWS,TKBANG,TKDQ,TKHASH,TKDS,TKPCS,TKAMP,TKSQ,
	TKLP,TKRP,TKSTAR,TKPLUS,TKCMA,TKMINUS,TKDOT,TKFSL,
	30,30,30,30,30,30,30,30,30,30,TKCOL,TKCOMMENT,TKLT,TKEQ,TKGT,0,
	33,30,30,30,30,30,30,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,37,44,38,0,0,
	0,30,30,30,30,30,30,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,39,0,40,0,0,
},
{
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
	34,34,35,34,34,34,34,34,34,34,34,34,34,34,34,34,
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
	34,34,34,34,34,34,34,34,34,34,34,34,36,34,34,34,
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
},
{
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
	34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,
},
{
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
	41,41,41,41,41,41,41,42,41,41,41,41,41,41,41,41,
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
	41,41,41,41,41,41,41,41,41,41,41,41,36,41,41,41,
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
},
{
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
	41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
}
};

struct lex_ctl {
	int nmo; // the new mode
	int nlc; // which table (above) to use next
	int tkn; // what token to make
	int mtkn; // -2 continuation, -1 clear token, >0 override token
};

// here be dragons
static struct lex_ctl wvtr[] = {
	{0, 0, 0},
	{1, 0, TKWS, -2},
	{0, 0, TKHASH},
	{28, 2, TKL},
	{30, 3, TKDEC, -2},
	{0, 0, TKBANG}, // 5
	{34, 4, TKDQ},
	{7, 0, TKDS},
	{28, 2, TKPCS},
	{0, 0, TKAMP},
	{41, 6, TKSQ}, // 10
	{0, 0, TKLP},
	{0, 0, TKRP},
	{0, 0, TKSTAR},
	{0, 0, TKPLUS},
	{0, 0, TKCMA}, // 15
	{0, 0, TKMINUS},
	{28, 2, TKDOT, -2},
	{0, 0, TKFSL},
	{28, 2, TKCOL, -2},
	{25, 0, TKWS}, // 20
	{0, 0, TKLT},
	{0, 0, TKGT},
	{0, 0, TKEQ},
	{24, 0, TKEOL},
	{25, 0, TKWS, -2}, // 25
	{26, 1, TKDEC, -2}, // 26 leading zero 0nn oct, 0xnn hex, 0bnn bin
	{30, 3, 0, TKHEX},
	{28, 2, 0, -2},
	{28, 2, 0, TKCOL},
	{30, 3, 0, -2}, // 30
	{30, 3, 0, TKOCT},
	{30, 3, 0, TKBIN},
	{0, 0, TKAT},
	{34, 4, 0, -2}, // in string
	{35, 0, TKDQ, -2}, // end string
	{34, 5, 0, -2}, // dq escape
	{0, 0, TKLBRK}, // 37
	{0, 0, TKRBRK},
	{0, 0, TKLBRC},
	{0, 0, TKRBRC}, // 40
	{41, 6, 0, -2},
	{42, 0, TKSQ, -2},
	{41, 7, 0, -2}, // sq escape
	{0, 0, TKBSL},
};

static void diag_color(const char * txt, size_t s, size_t e) {
	fprintf(stderr, "\e[31m");
	size_t x;
	for(x = s; x < e; x++) {
		fputc(txt[x], stderr);
	}
	fprintf(stderr, "\e[0m");
}

int wva_lex(void * wvas, char * text, size_t len) {
	size_t i = 0;
	char cc = 0;
	int mo = 0;
	int lc = 0;
	int mtkn = 0;
	int trsc;
	size_t tbegin, tend;
	const int psz = sizeof(wvtr) / sizeof(struct lex_ctl);
	while(i <= len) {
		cc = i == len ? 0 : text[i];
		switch(mo) {
		case TKCOMMENT:
			if(cc == 10 || cc == 13) {
				mo = 24;
				fputc(10, stderr);
				mtkn = TKEOL;
			}
			break;
		default:
			trsc = sttab[lc][cc];
			if(!trsc) {
				fprintf(stderr, "Invalid char: %d [%c]\n", cc, cc);
				return 1;
			} else {
				if(trsc < psz) {
					int nmo = wvtr[trsc].nmo;
					lc = wvtr[trsc].nlc;
					int nxtkn = wvtr[trsc].mtkn;
					int ntkn = wvtr[trsc].tkn;
					if(nxtkn > 0) {
						mtkn = nxtkn;
					} else if(nxtkn == -1) {
						mtkn = 0;
					}
					if(nmo != mo) {
						if(!ntkn && nxtkn > 0) {
						} else {
							if(mtkn && (nxtkn != -2 || mtkn != ntkn)) {
								fprintf(stderr, "{%d}", mtkn);
								tend = i;
								diag_color(text, tbegin, tend);
							}
							if(mtkn != ntkn) tbegin = i;
							mtkn = ntkn;
						}
						LEXDEBUG(fprintf(stderr, "[%d]%c", nmo, cc);)
						mo = nmo;
					} else {
						if(mtkn && (nxtkn == 0) && ntkn) {
							fprintf(stderr, "{|%d}", mtkn);
							tend = i;
							diag_color(text, tbegin, tend);
							tbegin = i;
							mtkn = ntkn;
						}
						LEXDEBUG(fputc(cc, stderr);)
					}
				} else {
					fprintf(stderr, "lex: Mode out of range\n");
					return -1;
				}
			}
			break;
		}
		i++;
		if(i >= len) {
			MAKETOKEN(EOL);
			break;
		}
	}
	return 0;
}

