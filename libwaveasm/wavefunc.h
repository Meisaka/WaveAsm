
#include <stdint.h>
#include <stdlib.h>

void * wva_alloc(size_t);
void * wva_realloc(void *, size_t);
void wva_free(void *);

int wva_lex(void * wvas, char * text, size_t len);
uint32_t wva_murmur3(const void * key, size_t len);

