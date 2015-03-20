
#include <stdio.h>
#include "wave.h"

int main(int argc, char* argv[], char** env)
{
	WVT_Bitset w;
	wva_alloc_bits(&w);
	wva_add_bits(w, 0x00a533e1, 24);
	printf("WaveAsm C v0.0.0\n");

	wva_show_bits(w);
	wva_add_bits(w, 0x74, 7); wva_show_bits(w);
	wva_add_bits(w, 0x6, 3); wva_show_bits(w);
	wva_add_bits(w, 0x77, 7); wva_show_bits(w);
	wva_add_bits(w, 0x77, 7); wva_show_bits(w);
	wva_add_bits(w, 0x77, 7); wva_show_bits(w);
	wva_add_bits(w, 0x77, 7); wva_show_bits(w);
	wva_add_bits(w, 0x77, 2); wva_show_bits(w);
	wva_free_bits(&w);
	return 0;
}

