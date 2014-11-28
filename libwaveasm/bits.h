
#include <stdint.h>

typedef struct WVS_Bitset {
	uint32_t nbits;
	uint32_t acum;
	uint32_t nset;
	uint32_t *bset;
} *WVT_Bitset;

void wva_show_bits(WVT_Bitset v);
void wva_add_bits(WVT_Bitset v, uint32_t b, int sz);
