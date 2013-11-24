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
WaveAsm currently supports 6 macros:
 - .ORG - setting where the code is generated for.
 - .EQU - Set the label on the line to whatever value is supplied.
 - .DAT - Encode values into minimally sized memory words in the endianess of the CPU
 - .DB - Same as .DAT currently
 - .DW - Similar to .DAT, except encode each value as a 16 bit Word
 - .DD - Similar to .DAT, except encode each value as a 32 bit Doubleword

Note:
.ORG generates code to run a at specified address, but the binary file is flat, so a relocator or specific load address will be needed.

Numeric Expressions
----
Numeric expressions are simple in the current version, they are single values and may be negative.
The following formats are used for numeric expressions:
 - 0xnn - Hexadecimal, where nn is any number hex digits.
 - nnh - Hexadecimal, alternate form.
 - 0bnn - Binary, nn is any number of bits.
 - 0nnn - Octal, nnn is any number of octal digits.
 - nn - Decimal, nn is any number of digits.
 - 'x' - Character, x is any single character, special escaped values are also supported \n \t \r \0 have standard meanings, use \' to embed the ' value.

Standard Syntax
----
WaveAsm uses a common assembly syntax, each line may be blank, a comment, just a label, or full line.
All labels must appear first on a line, before any white space.
All instructions must come after label and whitespace, whitespace before the instruction is required.
Arguments optionally follow the instruction, and are seperated with commas "," or depending on the instruction set, the plus sign "+".
The exact format of arguments is instruction set specific.
The default instruction set is [RC3200](https://github.com/Zardoz89/Trillek-Computer/blob/RC3200/RC3200.md).

Known bugs
----
This is still an early version, so line expressions are limited and isf syntax is strict.
Instruction set definitions are not syntax checked, they just define syntax, editing them could cause wierd behavior.

Unknown Bugs
----
There maybe other bugs. If you find any, let me know!

