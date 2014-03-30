WaveAsm
====

Overview
----

WaveAsm is an assembler that is designed to support all of the multiple CPU targets found in Trillek.
Each CPU format is defined by a special definition file containing all the instructions and how to encode them.

How to Use
----

The current version is written in perl, perl comes with all standard Unix and Linux distros
Running is simple, on Linux/Unix:

	./WaveAsm.pl somefile.to.asm

on Windows (perl must be installed separately):

	\Path\to\perl.exe WaveAsm.pl somefile.to.asm

This will generate two files:
 - somefile.to.lst - which contains the final listing.
 - somefile.to.ffi - a flat binary file in the endianess of the CPU target.

This will also print out a bit of debug info, along with any errors, add the '-v' flag to up verbosity, use multiple times for more nonsense output.
To use a different instruction set use:  `--A=<isfname>`, example: `--A=tr3200`
If the debug output does not have "complete!" at the end, check for errors, getting the arguments to an instruction wrong could print a "syntax error" or an "invalid addressing mode" error.

Supported Macros
----
Starting with version 0.3.0 macros and instructions are *not* case sensitive.
WaveAsm currently supports 6 macros:
 - .ORG - setting where the code is generated for.
 - .EQU - Set the label on the line to whatever value is supplied.
 - .DAT - Encode values into minimally sized memory words in the endianess of the CPU
 - .DB - Same as .DAT currently
 - .DW - Similar to .DAT, except encode each value as a 16 bit Word
 - .DD - Similar to .DAT, except encode each value as a 32 bit Doubleword

Note:
.ORG generates code to run a at specified address, but the default binary output format is flat, so a relocator or specific base load address will be needed.
Changing to a different address after one is already set with .ORG will fill the flat binary file with padding.

Expressions
----
Expressions are simple in the current version, they are values that can be added or subtracted and may be negative, numberic expressions are simplified from left to right, no form of precidence exists yet.
The following formats are used for expressions:
 - 0xnn - Hexadecimal, where nn is any number hex digits.
 - nnh - Hexadecimal, alternate form, be careful not mix these up with labels.
 - 0bnn - Binary, nn is any number of bits.
 - 0nnn - Octal, nnn is any number of octal digits.
 - nn - Decimal, nn is any number of digits.
 - 'x' - Character, x is any single character, special escaped values are also supported \n \t \r \0 have standard meanings, use \' to embed the ' value.
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
you can find a definition of [the TR3200 instruction set here](https://github.com/trillek-team/trillek-computer/blob/master/TR3200.md).
The [complete computer specs are here](https://github.com/trillek-team/trillek-computer/)

Other notes
----
This is still an in development version, the format of the isf is kind of strict to enfore proper operation.
Instruction set definitions are not syntax checked, they just define syntax, so editing them can cause wierd behavior, if done improperly, [a description of ISF](ISF.md) is availble, but not complete nor fully implemented.

Bugs / Feature Requests
----
If you find a bug, you can let me know in a github issue, I try to stay on top of these if I find any.
Or if want to request a feature that does not already have an issue, I am open to suggestions for improvements.

