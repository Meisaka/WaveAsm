/****
 * WaveAsm wavefunc.c - platform interface functions
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
#include "wavefunc.h"
#include <string.h>

void * wva_alloc(size_t s)
{
	void * n = malloc(s);
	if(n) memset(n, 0, s);
	return n;
}

void * wva_realloc(void *p, size_t s)
{
	return realloc(p, s);
}

void wva_free(void *p)
{
	free(p);
}

char * wva_alloc_file(size_t s)
{
	return (char*)wva_alloc(s);
}

