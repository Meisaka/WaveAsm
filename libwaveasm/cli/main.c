
#include <stdio.h>
#include "wave.h"

int main(int argc, char* argv[], char** env)
{
	struct WVS_Bitset w;
	w.bset = 0;
	w.nset = 0;
	w.acum = 0x00a533e1;
	w.nbits = 24;
	printf("WaveAsm C v0.0.0\n");

	wva_show_bits(&w);
	wva_add_bits(&w, 0x77, 7);
	wva_show_bits(&w);
	wva_add_bits(&w, 0x6, 3);
	wva_show_bits(&w);
	return 0;
}

