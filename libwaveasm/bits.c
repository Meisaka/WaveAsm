/****
 * WaveAsm bits.c - bit set edit functions
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
#include "bits.h"
#include "wavefunc.h"

#include <stdio.h>

void wva_alloc_bits(WVT_Bitset *v)
{
	void *mem;
	WVT_Bitset bt;
	if(v == 0) return;
	mem = wva_alloc(sizeof(struct WVS_Bitset));
	if(mem == 0) {
		*v = 0;
		return;
	}
	*v = bt = (WVT_Bitset)mem;
	bt->nbits = 0;
	bt->acum = 0;
	bt->nset = 0;
	bt->bset = 0;
}

void wva_reset_bits(WVT_Bitset v)
{
}

void wva_free_bits(WVT_Bitset *v)
{
	if(v == 0) return;
	if(*v == 0) return;
	if((*v)->bset) {
		wva_free((*v)->bset);
	}
	wva_free(*v);
	*v = 0;
}

void wva_show_bits(WVT_Bitset v)
{
	uint32_t blocks;
	uint32_t m, sa;
	int i, k;
	if(v == 0) return;
	blocks = (v->nbits - 1) >> 5;
	fprintf(stderr, "BC: %d/%d \n", v->nbits, blocks);
	m = v->nbits - 1;
	for(k = blocks; k >= 0; k--) {
		if(k) {
			sa = v->bset[blocks - k];
			i = 31;
		} else {
			sa = v->acum;
			i = m & 31;
		}
		for(; i >= 0; i--) {
			putc('0' + ((sa >> i) & 1), stdout);
			if(((i) & 7) == 0) putc(' ', stdout);
		}
		m -= 32;
	}
	putc('\n', stdout);
}

void wva_add_bits(WVT_Bitset v, uint32_t b, int sz)
{
	uint32_t cl, oa, na, bc;
	uint32_t *nm;
	int shf, shr;
	if(v == 0) return;
	cl = b & ((1 << sz) - 1);
	printf("Add: %08x x %d\n", cl, sz);
	if((v->nbits & 31) + sz <= 32) {
		v->nbits += sz;
		v->acum <<= sz;
		v->acum |= cl;
	} else {
		shf = 32 - (v->nbits & 31);
		shr = sz - shf;
		oa = (v->acum << shf) | (cl >> shr);
		na = cl & ((1 << shr) - 1);
		// uhh
		printf("S: %d m: %08x\n", shf, ((1 << shf) - 1));
		printf("A1: %08x\nA2: %08x\n", oa, na);
		v->nbits += sz;
		bc = v->nbits >> 5;
		printf("BC: %d/%d\n", v->nbits, bc);
		if(!v->bset) {
			nm = (uint32_t*)wva_alloc(sizeof(uint32_t) * 2);
			v->bset = nm;
			v->nset = 2;
		} else if(v->nset < bc) {
			v->nset += 2;
			nm = (uint32_t*)wva_realloc(v->bset, sizeof(uint32_t) * v->nset);
			v->bset = nm;
		}
		v->bset[bc - 1] = oa;
		v->acum = na;
	}
}

int wva_write_bits_le(WVT_Bitset v, uint8_t *buf, size_t sz)
{
	return 0;
}

int wva_write_bits_be(WVT_Bitset v, uint8_t *buf, size_t sz)
{
	return 0;
}

