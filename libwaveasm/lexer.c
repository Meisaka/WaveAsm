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

#define MAKETOKEN(a) fprintf(stderr, "Tkn-" #a)

#define TKWS 1
#define TKHASH 2
#define TKL 3
#define TKN 4
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

static int sttab[256] = {
	0,0,0,0,0,0,0,0,0,0,TKEOL,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	TKWS,TKBANG,TKDQ,TKHASH,TKDS,TKPCS,TKAMP,TKSQ,
	TKLP,TKRP,TKSTAR,TKPLUS,TKCMA,TKMINUS,TKDOT,TKFSL,
	TKN,TKN,TKN,TKN,TKN,TKN,TKN,TKN,TKN,TKN,TKCOL,TKSCOL,TKLT,TKEQ,TKGT,0,
	0,TKL,TKL,TKL,TKL,TKL,TKL,TKL,TKL,TKL,TKL,TKL,TKL,TKL,TKL,TKL,
	TKL,TKL,TKL,TKL,TKL,TKL,TKL,TKL,TKL,TKL,TKL,0,0,0,0,TKL,
	0,TKL,TKL,TKL,TKL,TKL,TKL,TKL,TKL,TKL,TKL,TKL,TKL,TKL,TKL,TKL,
	TKL,TKL,TKL,TKL,TKL,TKL,TKL,TKL,TKL,TKL,TKL,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

int wva_lex(void * wvas, char * text, size_t len) {
	size_t i = 0;
	char cc = 0;
	int mo = 0;
	int trsc;
	while(i <= len) {
		cc = i == len ? 0 : text[i];
		trsc = sttab[cc];
		if(!trsc) {
			fprintf(stderr, "Invalid char: %d [%c]\n", cc, cc);
			return 1;
		} else {
			fprintf(stderr, "Mode: [%c]%d\n", cc, trsc);
		}
		i++;
		if(i >= len) {
			MAKETOKEN(EOL);
			break;
		}
	}
	return 0;
}

