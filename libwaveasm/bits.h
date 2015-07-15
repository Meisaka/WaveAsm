#ifndef WAVEASM_BITS_INCL
#define WAVEASM_BITS_INCL

#include <stdint.h>
#include <stdlib.h>

typedef struct WVS_Bitset {
	uint32_t nbits;
	uint32_t acum;
	uint32_t nset;
	uint32_t *bset;
} *WVT_Bitset;

void wva_alloc_bits(WVT_Bitset *v);
void wva_reset_bits(WVT_Bitset v);
void wva_free_bits(WVT_Bitset *v);
void wva_show_bits(WVT_Bitset v);
void wva_add_bits(WVT_Bitset v, uint32_t b, int sz);
int wva_write_bits_le(WVT_Bitset v, uint8_t *buf, size_t sz);
int wva_write_bits_be(WVT_Bitset v, uint8_t *buf, size_t sz);

#endif

