/****
 * WaveAsm CLI main.c - entry for standalone assembler
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
#include <stdio.h>
#include "wave.h"

static int load_file_callback(const char *file, int search, char **data, size_t *datalen, void * usr)
{
	FILE *f = fopen(file, "r");
	if(!f) return -1;
	fseek(f, 0, SEEK_END);
	off_t sz = ftello(f);
	fseek(f, 0, SEEK_SET);
	*data = wva_alloc_file(sz);
	*datalen = fread(*data, 1, sz, f);
	fclose(f);
	return 0;
}

// fn = file name
// rsptr is caller freed if set by this function
static int load_file(char * fn, char **rsptr, size_t *rssz) {
	if(!rsptr || !rssz) {
		return -5;
	}
	if(*rsptr) {
		return -5;
	}
	fprintf(stderr, "Loading %s\n", fn);
	FILE *f = fopen(fn, "r");
	if(!f) {
		perror("file open");
		return -1;
	}
	size_t rdlen;
	fseek(f, 0, SEEK_END);
	rdlen = ftello(f);
	fseek(f, 0, SEEK_SET);
	size_t flen = 0;
	char *ft = (char*)malloc(rdlen);
	if(!ft) {
		perror("malloc");
		fclose(f);
		return -1;
	}
	flen = fread(ft, 1, rdlen, f);
	fclose(f);
	fprintf(stderr, "Read: %d bytes\n", flen);
	*rsptr = ft;
	*rssz = flen;
	return 0;
}

int main(int argc, char* argv[], char** env)
{
	fprintf(stderr, "WaveAsm C v0.0.1\n");
	wvat_state wave;
	char *isf = 0;
	char *x = 0;
	size_t xl = 0;
	if(argc > 1) {
		wva_allocstate(&wave);
		wave->onload = load_file_callback;
		if(load_file("dcpu16.isf", &isf, &xl)) {
		//if(load_file("tr3200.isf", &isf, &xl)) {
			return 2;
		}
		wva_loadisf(wave, isf, xl);
		if(load_file(argv[1], &x, &xl)) {
			return 2;
		}
		if(!x) return 1;
		wvat_obj obj;
		wva_assemble(wave, &obj, x, xl);
		wva_freestate(wave);
	}
	return 0;
}

