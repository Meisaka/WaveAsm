WaveAsm
====

Overview
----

WaveAsm is a simple assembler that is designed to support multiple CPU targets.
Each CPU format is defined by a special definition file containing all the instructions and how to encode them.

How to Use
----

The current version is written in perl, perl comes with all standard Unix and Linux distros
Running is simple, on Linux/Unix:

	./WaveAsm.pl somefile.to.asm

on Windows:

	\Path\to\perl.exe WaveAsm.pl somefile.to.asm

This will generate two files:
 - somefile.to.asm.lst - which contains the final listing.
 - somefile.to.asm.bin - a flat binary file in the endieness of the CPU target.

This will also print out quite a bit of debug info, along with any errors.
If the debug output does not have "complete!" at the end, check for errors.

Supported Macros
----
WaveAsm supports 2 macros at the moment:
 - .ORG - setting where the code is generated for.
 - .EQU - Set the label on the line to whatever value is supplied.

Note:
.ORG generates code to run a at specified address, but the binary file is flat, so a relocator or specific load address will be needed.

Known bugs
----
This an early version, so line expressions are very limited and strict.
Instruction set definitions are not syntax checked, they just define syntax, editing them could cause wierd behavior.
Numeric expressions are limited to decimal and hex (prefixed with 0x)

Unknown Bugs
----
There maybe other bugs. If you find any, let me know!

