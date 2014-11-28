
#include "bits.h"

#include <stdio.h>

void wva_show_bits(WVT_Bitset v)
{
	if(v == 0) return;

	uint32_t blocks = v->nbits >> 5;
	fprintf(stderr, "BC: %d/%d \n", v->nbits, blocks);
	uint32_t m, sa;
	int i;
	m = v->nbits;
	for(i = m - 1, sa = v->acum; i >= 0; i--) {
		putc('0' + ((sa >> i) & 1), stdout);
		if(((i) & 3) == 0) putc(' ', stdout);
	}
	putc('\n', stdout);
}

void wva_add_bits(WVT_Bitset v, uint32_t b, int sz)
{
	if(v == 0) return;
	uint32_t cl = b & ((1 << sz) - 1);
	printf("Add: %08x x %d\n", cl, sz);
	if((v->nbits & 31) + sz <= 32) {
		v->nbits += sz;
		v->acum <<= sz;
		v->acum |= cl;
	} else {
		int shf = 32 - (v->nbits & 31);
		int shr = sz - shf;
		uint32_t oa = (v->acum << shf) | (cl >> shr);
		uint32_t na = cl & ((1 << shr) - 1);
		// uhh
		printf("S: %d m: %08x\n", shf, ((1 << shf) - 1));
		printf("A1: %08x\nA2: %08x\n", oa, na);
		v->acum = oa;
		v->nbits = 32;
	}
}

