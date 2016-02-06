
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
void * wva_alloc(size_t);
void * wva_realloc(void *, size_t);
void wva_free(void *);

int wva_lex(void * wvas, void *wvafs, char * text, size_t len);

int strneq(const char *, size_t, const char *, size_t);
#ifdef __cplusplus
}
#endif

