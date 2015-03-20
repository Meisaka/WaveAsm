
#include "wavefunc.h"

void * wva_alloc(size_t s)
{
	return malloc(s);
}

void * wva_realloc(void *p, size_t s)
{
	return realloc(p, s);
}

void wva_free(void *p)
{
	free(p);
}

