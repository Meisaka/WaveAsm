#ifndef WAVEASM_HASH_INCL
#define WAVEASM_HASH_INCL

#ifdef __cplusplus
#define WAVEASMC extern "C"
#else
#define WAVEASMC
#endif

#include <stdint.h>
#include <stdlib.h>

WAVEASMC
uint32_t wva_murmur3(const void * key, size_t len);

#endif

