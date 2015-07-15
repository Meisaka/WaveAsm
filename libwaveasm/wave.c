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
#include "wave.h"
#include "wavefunc.h"
#include "setting.h"

#include <stdio.h>

int strneq(const char *a, size_t al, const char *b, size_t bl)
{
	if(al != bl) return 0;
	return strncmp(a, b, al) == 0;
}

typedef struct wvai_state {
	uint32_t nplatforms;
	uint32_t nls;
} *wvai_state;

int wva_allocstate(wvat_state *st)
{
	return 0;
}

int wva_freestate(wvat_state st)
{
	return 0;
}

int wva_loadisf(wvat_state st, char *isftxt, size_t isflen)
{
	size_t x, e;
	int mode;
	x = 0; mode = 0;
	while(x < isflen) {
		for(e = x; e < isflen && isftxt[e] != 10; e++); e;
		switch(mode) {
		case 0:
			if(isftxt[x] == '<') {
				fprintf(stderr, "Section: %d-%d\n", x, e-x);
			}
			if(strneq(isftxt+x, e-x, "<OPCODE>", 8)) {
				fprintf(stderr, "OPCODE section\n");
				mode = 1;
			}
			break;
		case 1:
			if(strneq(isftxt+x, e-x, "</OPCODE>", 9)) {
				fprintf(stderr, "End OPCODE section\n");
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
	wva_lex(st, atxt, len);
	return 0;
}

int wva_objfree(wvat_obj ob)
{
	return 0;
}

