/***
 * wave.h - Main include for the WaveAsm library
 *
 * Copyright 2015 Meisaka Yukara
 *
 */
#ifndef WAVEASM_INCL
#define WAVEASM_INCL

#include <stdint.h>
#include <stdlib.h>

#include "bits.h"

#ifdef __cplusplus
extern "C" {
#endif

/* allocate functions for loading file */
char * wva_alloc_file(size_t);

/* load file, allocate "data" with wva_alloc_file, the assembler will free automatically when done. */
typedef int (*wva_loadfilecallback)(const char *file, int search, char **data, size_t *datalen, void * usr);

typedef
struct wvas_state {
	void *in_state;
	void *isf_state;
	void *lc_state;
	wva_loadfilecallback onload;
	void *onload_usr;
} *wvat_state;

typedef
struct wvas_obj {
	size_t len;
} *wvat_obj;

/* create and destroy WaveAsm instances */
int wva_allocstate(wvat_state *);
int wva_freestate(wvat_state);

/* load ISF from text data */
int wva_loadisf(wvat_state, char *, size_t);

/* load ISF from compiled data */
int wva_loadisfc(wvat_state, uint8_t *, size_t);

/* assemble object text */
int wva_assemble(wvat_state, wvat_obj *, char *, size_t);

/* free object data used by WaveAsm */
int wva_objfree(wvat_obj);
#ifdef __cplusplus
}
#endif
#endif

