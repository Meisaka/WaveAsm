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
	0,0,0,0,0,0,0,0,0,1,TKEOL,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,TKBANG,TKDQ,TKHASH,TKDS,TKPCS,TKAMP,TKSQ,
	TKLP,TKRP,TKSTAR,TKPLUS,TKCMA,TKMINUS,TKDOT,TKFSL,
	26,4,4,4,4,4,4,4,4,4,TKCOL,25,TKLT,TKEQ,TKGT,0,
	33,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,37,44,38,0,3,
	0,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,39,0,40,0,0,
},
{
	0,0,0,0,0,0,0,0,0,1,TKEOL,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,TKBANG,TKDQ,TKHASH,TKDS,TKPCS,TKAMP,TKSQ,
	TKLP,TKRP,TKSTAR,TKPLUS,TKCMA,TKMINUS,TKDOT,TKFSL,
	31,31,31,31,31,31,31,31,0,0,TKCOL,25,TKLT,TKEQ,TKGT,0,
	33,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,27,0,0,37,44,38,0,0,
	0,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,27,0,0,39,0,40,0,0,
},
{
	0,0,0,0,0,0,0,0,0,1,TKEOL,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,TKBANG,TKDQ,TKHASH,TKDS,TKPCS,TKAMP,TKSQ,
	TKLP,TKRP,TKSTAR,TKPLUS,TKCMA,TKMINUS,28,TKFSL,
	28,28,28,28,28,28,28,28,28,28,29,25,TKLT,TKEQ,TKGT,0,
	33,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,
	28,28,28,28,28,28,28,28,28,28,28,37,44,38,0,28,
	0,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,
	28,28,28,28,28,28,28,28,28,28,28,39,0,40,0,0,
},
{
	0,0,0,0,0,0,0,0,0,1,TKEOL,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,TKBANG,TKDQ,TKHASH,TKDS,TKPCS,TKAMP,TKSQ,
	TKLP,TKRP,TKSTAR,TKPLUS,TKCMA,TKMINUS,TKDOT,TKFSL,
	30,30,30,30,30,30,30,30,30,30,TKCOL,25,TKLT,TKEQ,TKGT,0,
	33,30,30,30,30,30,30,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,37,44,38,0,0,
	0,30,30,30,30,30,30,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,39,0,40,0,0,
},
{ // 4
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
{ // 6
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
},
{ // 8
	25,25,25,25,25,25,25,25,25,25,24,25,25,24,25,25,
	25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
	25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
	25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
	25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
	25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
	25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
	25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
	25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
	25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
	25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
	25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
	25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
	25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
	25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
	25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
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
	{28, 2, TKHASH, -2},
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
	{25, 8, TKWS, -2}, // 25
	{26, 1, TKDEC, -2}, // 26 leading zero ; 0nn oct, 0xnn hex, 0bnn bin
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

void lex_add_token(void * state, void *fs, const char * text, size_t s, size_t e, int tk);

int wva_lex(void * wvas, void *fs, char * text, size_t len) {
	size_t i = 0;
	uint8_t cc = 0;
	int mo = 0;
	int lc = 0;
	int mtkn = 0;
	int lmtkn = 0;
	int trsc;
	int line = 1;
	int col = 0;
	int cif = 0;
	size_t tbegin, tend;
	tbegin = 0;
	tend = 0;
	const int psz = sizeof(wvtr) / sizeof(struct lex_ctl);
	while(i <= len) {
		cc = i == len ? 32 : (uint8_t)text[i];
		if(col == 0) {
			LEXDEBUG(fprintf(stderr, "Line %d:", line));
		}
		trsc = sttab[lc][cc];
		if(!trsc) {
			fprintf(stderr, "\nInvalid char: %d [%c]\nmo=%d, lc=%d\n", cc, cc, mo, lc);
			return 1;
		}
		if(trsc >= psz) {
			fprintf(stderr, "lex: Mode out of range\n");
			return -1;
		}
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
					LEXDEBUG(fprintf(stderr, "{%d}", mtkn);)
					lmtkn = mtkn;
					tend = i;
					lex_add_token(wvas, fs, text, tbegin, tend, mtkn);
					if(mtkn != 1) cif = 1;
					if(mtkn == 24) cif = 0;
				}
				if(mtkn != ntkn) tbegin = i;
				mtkn = ntkn;
			}
			LEXDEBUG(fprintf(stderr, "[%d]%c", nmo, cc);)
			mo = nmo;
		} else {
			if(mtkn && (nxtkn == 0) && ntkn) {
				LEXDEBUG(fprintf(stderr, "{|%d}", mtkn);)
				tend = i;
				lmtkn = mtkn;
				lex_add_token(wvas, fs, text, tbegin, tend, mtkn);
				tbegin = i;
				mtkn = ntkn;
				if(mtkn != 1) cif = 1;
				if(mtkn == 24) cif = 0;
			}
			LEXDEBUG(fputc(cc, stderr);)
		}
		/*
		if(!cif && cc == '#') { // # at beginning of lines as comments
			mo = 25;
			trsc = 25;
			mtkn = 1;
			lc = wvtr[trsc].nlc;
		}*/
		col++;
		if(lmtkn == 24) {
			line ++;
			col = 0;
			cif = 0;
		}
		lmtkn = 0;
		i++;
	}
	return 0;
}

