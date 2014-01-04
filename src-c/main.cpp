
#include <FlexLexer.h>
#include <iostream>
#include <fstream>
#include "bitsys.hpp"

int main(int argc, char **argv, char **env)
{
	yyFlexLexer* lex = new yyFlexLexer();
	++argv; --argc;
	if(argc > 0) {
		lex->switch_streams(new std::ifstream(argv[0]));
	}
	//lex->yylex();
	BitString xbt;
	xbt << 'A' << 'B';
	std::cout << xbt.GetByteStringBE() << '\n';
	xbt.AddRight(0x12, 8);
	std::cout << xbt.GetByteStringBE() << '\n';
	xbt.AddRight(0x34, 8);
	std::cout << xbt.GetByteStringBE() << '\n';
	xbt.AddRight(0x56789ABC, 32);
	std::cout << xbt.GetByteStringBE() << '\n';
	xbt.AddRight(0x0f, 2);
	std::cout << xbt.GetByteStringBE() << '\n';
	xbt.AddRight(0x0f, 6);
	std::cout << xbt.GetByteStringBE() << '\n';
	xbt.AddRight(0x8AAA, 16);
	std::cout << xbt.GetByteStringBE() << '\n';
	xbt.AddRight(0x85A5, 16);
	std::cout << xbt.GetByteStringBE() << '\n';
	return 0;
}

