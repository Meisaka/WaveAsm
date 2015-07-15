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

typedef
struct wvas_state {
	void *inner_state;
	void *isf_state;
	void *lc_state;
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

#endif

