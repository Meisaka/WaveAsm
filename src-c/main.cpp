
#include <FlexLexer.h>
#include <iostream>
#include <fstream>
#include "bitsys.hpp"

int main(int argc, char **argv, char **env)
{
	// We're just testing individual parts right now
	yyFlexLexer* lex = new yyFlexLexer();
	++argv; --argc;
	if(argc > 0) {
		lex->switch_streams(new std::ifstream(argv[0]));
	}
	//lex->yylex(); // test lex
	
	// test bit string class
	// bits are always aligned along the 32 bit "right edge" and aligned as bytes by the GetHex functions.
	BitString xbt;
	xbt << 'A' << 'B' << 'C';
	std::cout << xbt.GetHexBE() << '\n' << xbt.GetHexLE() << '\n';
	xbt.AddRight(0x12, 8);
	std::cout << xbt.GetHexBE() << '\n' << xbt.GetHexLE() << '\n';
	xbt.AddRight(0x34, 8);
	std::cout << xbt.GetHexBE() << '\n' << xbt.GetHexLE() << '\n';
	xbt.AddRight(0x56789ABC, 32);
	std::cout << xbt.GetHexBE() << '\n' << xbt.GetHexLE() << '\n';
	xbt.AddRight(0x0f, 2);
	std::cout << xbt.GetHexBE() << '\n' << xbt.GetHexLE() << '\n';
	xbt.AddRight(0x0f, 6);
	std::cout << xbt.GetHexBE() << '\n' << xbt.GetHexLE() << '\n';
	xbt.AddRight(0x8AAA, 16);
	std::cout << xbt.GetHexBE() << '\n' << xbt.GetHexLE() << '\n';
	xbt.AddRight(0x85A5, 16);
	std::cout << xbt.GetHexBE() << '\n' << xbt.GetHexLE() << '\n';
	xbt.clear();
	xbt >> 'A' >> 'B' >> 'C';
	std::cout << xbt.GetHexBE() << '\n' << xbt.GetHexLE() << '\n';
	xbt.AddLeft(0x12, 8);
	std::cout << xbt.GetHexBE() << '\n' << xbt.GetHexLE() << '\n';
	xbt.AddLeft(0x34, 8);
	std::cout << xbt.GetHexBE() << '\n' << xbt.GetHexLE() << '\n';
	xbt.AddLeft(0x56789ABC, 32);
	std::cout << xbt.GetHexBE() << '\n' << xbt.GetHexLE() << '\n';
	xbt.AddLeft(0x0f, 2);
	std::cout << xbt.GetHexBE() << '\n' << xbt.GetHexLE() << '\n';
	xbt.AddLeft(0x0f, 6);
	std::cout << xbt.GetHexBE() << '\n' << xbt.GetHexLE() << '\n';
	xbt.AddLeft(0x8AAA, 16);
	std::cout << xbt.GetHexBE() << '\n' << xbt.GetHexLE() << '\n';
	xbt.AddLeft(0x85A5, 16);
	std::cout << xbt.GetHexBE() << '\n' << xbt.GetHexLE() << '\n';
	xbt.clear();
	xbt.AddRight(0x81,8);
	xbt.AddRight(0, 14);
	xbt.AddRight(9, 5);
	xbt.AddRight(7, 5);
	std::cout << xbt.GetHexLE() << '\n';
	return 0;
}

