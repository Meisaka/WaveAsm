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
	wvat_hashtab ht;
	struct wvas_hashentry *hte;
	wva_allochash(&ht);
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
			if(0 != (rc = memchr(line, ':', ll))) {
				if(rc - line > MAXOPCODELEN) {
					fprintf(stderr, "ISF: Opcode too long.\n");
					break;
				}
				memtolower(opc, line, rc-line);
				opc[rc-line+1] = 0;
				fprintf(stderr, "ISF: Opcode: %s\n", opc);
				hte = wva_lookuphash(ht, opc, rc-line);
				if(!hte) {
					fprintf(stderr, "ISF: Adding opcode\n");
					wva_addhash(ht, opc, rc-line, rc, ll - (rc-line));
				} else {
					fprintf(stderr, "ISF: Adding encoding\n");
					wva_addhash(ht, opc, rc-line, rc, ll - (rc-line));
					write(2, hte->data, hte->datalen);
				}
			}
			break;
		default:
			mode = 0;
			break;
		}
		x = e + 1;
	}
	wva_freehash(ht);
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

