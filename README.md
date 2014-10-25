WaveAsm
====

Overview
----

WaveAsm is a re-targetable assembler for the TR3200 CPU found in Trillek.
CPU formats are defined by a special definition file containing all the instructions and how to encode them.

Usage
----

The current version is written in perl, perl comes with all standard Unix and Linux distros
Running is simple, on Linux/Unix:

	./WaveAsm.pl somefile.to.asm

on Windows (perl must be installed separately):

	\Path\to\perl.exe WaveAsm.pl somefile.to.asm

This will generate two files:
 - somefile.to.lst - which contains the final listing.
 - somefile.to.ffi - a flat binary file in the endianess of the CPU target.

This will also print out a little status info, along with any errors, add the '-v' flag to up verbosity, use multiple times for more nonsense output.
To use a different instruction set use:  `--A=<isfname>`, example: `--A=tr3200`
If the output does not have "complete!" at the end, check for errors, getting the arguments to an instruction wrong typically prints a "syntax error" or an "invalid addressing mode" error.

Supported Macros
----
Built-in macros and instructions are *not* case sensitive.
WaveAsm currently supports 6 macros:
 - .ORG - setting where the code is generated for.
 - .EQU - Set the label on the line to whatever value is supplied.
 - .DATA - Encode values into minimally sized memory words in the endianess of the CPU
 - .DAT - alternate for .DATA
 - .DB - Same as .DATA currently
 - .DW - Similar to .DATA, except encode each value as a 16 bit Word
 - .DD - Similar to .DATA, except encode each value as a 32 bit Doubleword

Note:
.ORG generates code to run a at specified address, but the default output format is flat and does not support this, so a relocator or specific base load address will be needed.
Changing to a different address *after* one is already set with .ORG will fill the flat binary file with padding, if that address is higher in memory.

Expressions
----
Expressions are simple in the current version, they are values that can be added or subtracted and may be negative, numeric expressions are simplified from left to right, no form of precidence exists yet.
The following formats are used for expressions:
 - 0xnn - Hexadecimal, where nn is any number hex digits.
 - nnh - Hexadecimal, alternate form, be careful not mix these up with labels.
 - 0bnn - Binary, nn is any number of bits.
 - 0nnn - Octal, nnn is any number of octal digits.
 - nn - Decimal, nn is any number of digits.
 - 'x' - Character, x is any single character, special escaped values are also supported \n \t \r \0 have standard C meanings, use \' to embed the ' value.
 - "xxx" - String, only use with a data macro (.DAT .DB etc.), supports same escape scheme as character values.

Standard Syntax
----
WaveAsm uses a flexible assembly syntax, each line may be blank, a comment, just a label, or full line.
A label must appear before instructions or macros, a line may contain just a label.
A label may be declared with an optional colon ":" before or after it.
Instructions come after label (if any).
Arguments optionally follow the instruction, and are seperated with commas "," or other puctuation depending on the instruction set.
The exact format of arguments is instruction set specific and defined in the ISF.
The default instruction set is for the Trillek TR3200 CPU,
you can find a definition of [the TR3200 instruction set here](https://github.com/trillek-team/trillek-computer/blob/master/cpu/TR3200.md).
The [complete computer specs are here](https://github.com/trillek-team/trillek-computer/)

Other notes
----
This is still a development version, the format of ISF is strictly structured to enfore proper operation.
Instruction set definitions are not syntax checked, they just define syntax, so editing them can cause wierd behavior if done improperly. [A description of ISF](ISF.md) is availble, but not complete nor fully implemented.

Bugs / Feature Requests
----
If you find bugs (I know they exist), please report them, I can not fix them unless I know, be sure to include whatever is needed to reproduce.
If you want to request a feature, let me know, I am open to suggestions to improve the usefulness of this program.

