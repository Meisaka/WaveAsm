WaveAsm Instruction Set File (ISF) Format - Versions v1 & v2
====

Conventions and Overview
----

Each ISF is a text file containing the both the syntax of opcodes and their encodings. Each ISF defines a complete instruction set for a particular CPU type.

- ISF v2 is reverse compatible with v1.
- ISF v1 is only partially forward compatible with v2.
- This document mostly describes only v1 !
- ISF text files use the ASCII linefeed character to delimit lines.
- whitespace consists of either ASCII character SP(32) or TAB(9)
- strings in ISF are text bound by ASCII "(34), an escape sequence \" is used to embed a double quote in a string.

Much TODO

Sections and Comments
----
An ISF is broken up into sections. A section is defined to be everything from the section start marker to the section end marker.

Format of markers:

    StartMarker := "<" SectionName ">" EOL
    EndMarker := "<" "/" SectionName ">" EOL

A section marker must be the only thing on a line to be valid.
Section names are case sensitive, so "HEAD" and "head" are considered different sections.

Any line in an ISF can be comment line, a comment line starts with "#" and extends to the end of the line, comments may be preceded by whitespace.

Encoder Syntax
----

Numbers, example and sizes:

- %000 binary, MSB first
- 0x00 hex, each hex digit is 4 bits
- 0533 octal, each digit is 3 bits
- 123 decimal, whole is compressed to smallest number of bits

consists of sequences separated by SP.
each sequence MUST contain a concatenate "+" character, even if it contains only a single element (v1).
Multiple elements in a sequence are seperated the "+" character.
Each sequence is encoded from left to right, MSB to LSB into memory words, sequences larger than a memory word are stored in the endianness of the CPU.

### section specific
In the OPCODE section there is an additional back reference value, that is a backslash "\" followed by a 1 indexed decimal number. The back reference is replaced by the encoding of name it indexes from the "format" field.

In the LIT section there is the current value reference "*", it is normally treated as a decimal number.

### command values

- Set encode length: "L" number(length bits)
- Append sequence of length: "AL" number(length bits) EncoderValue

HEAD section
----
Lines in the HEAD section are key-value pairs seperated by the ":" character.

- ISN - string - The instruction set identifier, used on command line or a file directive to select this ISF, this value must be unique across all installed ISFs.
- ISD - string - A short description of the instruction set.
- CPUE - enum - Endianness of the CPU being defined, one of "LE" "BE" "NE" for Little, Big, or No endian respectively.
- CPUM - number - The memory word size of the CPU in bits, defines how many bits each address holds.
- CPUW - number - The bit width of the CPU architecture.
- FILEE - enum - Endianness of binary output files, either "LE" or "BE"
- FILEW - number - bit size of binary file data (8 by default), should not be changed in most cases.
- ALIGN - number - modulus that opcodes are aligned to in memory words, 0 or 1 (defaults) means no alignment.

KEYWORD Section
----

Keywords are identifiers reserved by the instruction set to indicate alternate/special encodings or usages, or to act as meta data for reflection.

    Attribtype := char*1
    Attrib := Attribtype value
    Attribs := "+" Attrib ["," Attrib]*
    NameAttrib := "N" string(name)
    KeywordName := ( LETTER | "_" ) (LETTER | "_")*
    KeywordDef := KeywordName [ ":" Encoding ]

REG Section
----

Defines and describes registers in an instruction set. Register names are treated as special keywords, and are **NOT** case sensitive.

    Attribtype := char*1
    Attrib := Attribtype value
    Attribs := "+" Attrib ["," Attrib]*
    NameAttrib := "N" string(name)
    LengthAttrib := "L" number(length bits)
    IndexAttrib := "I" number(index bits)
    DataAttrib := "D" number(data bits)
    RegName := ( LETTER | "%" | "_" ) (LETTER | DIGIT | "%" | "_")*
    RegDef := RegName ["," RegName ...] [":" Encoding ]

LIT Section
----

This section defines literal values, and specifies how constant values are encoded into instructions that may support them.
Literals are defined by their length in bits and by one or more encoding definitions.
Each encoding definition may specify a range or ```*``` to match any number.
The order of lines with each definition is significant, the most specific/smallest ranges must come before larger or more general ranges.
The ```*``` line (if any) MUST be last in the list.

The special "O" and "M" attributes allow simple trasforms to the number before encoding, "O" will add to the given value, "M" will perform an arithmatic right shift. "O" is always performed before "M" if both are set.

The "S" attribute, indicates that the value (up to specified bit size) can be treated as signed by the encoder and will be sign-extended when decoded and evaluated.
Used to indicate that large unsigned values may be converted to signed numbers, that may end up with a low magnitude, match a better literal range, and ultimately be encoded more efficiently.

    Attribtype := char*1
    Attrib := Attribtype + value
    Attribs := "+" Attrib ["," Attrib]*
    NameAttrib := "N" + string(LitName)
    LengthAttrib := "L" + number(length)
    SignedAttrib := "S" + number(eval bits)
    OffsetAttrib := "O" + number(add)
    ShiftAttrib := "M" + number(right shift bits)
    LitVal := "*" | number(inclusive range)
    LitDef := LitVal(lower) ["," LitVal(upper)] [":" Encoding [":" Encoding ...] ]

GROUP Section
----
Groups define a single name to a number of different argument formats, and their encodings.
Encoding for a group behaves the same as an Opcode (using a back reference).
Paramformat is simialar to an Opcode, but only allows one param.
Groupnames are intended to be used a few times, "grouping" things together under the same name.

Format overview:

    format := ( [LitName | RegName | "#" | "[" | "]" | "(" | ")" | "+"] )*
    paramformat := string(format)
	Group := string(GroupName) ":" paramformat ":" Encoding

OPCODE Section
----
Defines Opcodes, their argument format (if any), and encoding.
For relative mode opcodes, all literal values are treated as an address and converted to an offset before scanning LIT ranges.

Format overview:

    format := ( [LitName | RegName | "," | "#" | "[" | "]" | "(" | ")" | "+"] )*
    paramformat := string(format)
    OpName := ( LETTER | "_" ) (LETTER | DIGIT | "." | "_")*
    paramsv1 := [ number(argument count) ] [ "," "r"(relative mode) ]
    params := paramsv1 | paramsv2
    Opcode := OpName ":" params[,params...] ":" paramformat ":" Encoding

