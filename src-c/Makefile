CFLAGS=-g 
CXXFLAGS=-g 

WaveAsm: main.o lexr.o bitsys.o
	g++ ${CFLAGS} -o WaveAsm lexr.o main.o bitsys.o

lex.yy.cc: wave.l Makefile
	flex -c++ wave.l

lexr.o: lex.yy.cc
	g++ -o lexr.o -c lex.yy.cc

bitsys.o: bitsys.cpp bitsys.hpp Makefile

main.o: main.cpp Makefile bitsys.hpp

