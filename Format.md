WaveAsm output formats
====

Format binary Flat [-ff] (default)
----

Flat binary format, contains no formatting, metadata or other info.
The program code is placed in the file in the order it is assembled.

When assembling flat files, any includes normally should go at the bottom of the file, this is only a recommendation.

The use of .org macros will change the generation address, but not the order of the code in the binary file.

Format text S19 [-fs]
----

Simple text based format that allows addressable loading and a program entry address.

All values in s19 are uppercase ASCII and numbers,
data are stored as sequential bytes (big endian) as two hex digits each.

All data are broken into "records" which can contain up to 254 bytes each, typical records will be shorter.
Each record consists of: ASCII "S", a digit indicating record type, a byte length - 2 hex digits, variable size fields.

The first field is the address field, it is normally 2 bytes long,
3 bytes for S2,S8 records, 4 bytes for S3 and S9 records.

Next is the data field, the size of the data field is (length - 1) - address_bytes

After the data field is a one byte checksum field,
this is computed by adding up the length and all other fields,
taking the least significant byte and inverting the bits (ones complement).

The file is a sequence of records, seperated by newlines. All records must have a checksum.
These are the record types:

- S0 metadata or version string
- S1,S2,S3  actual program data and addresses (which kind depends on address size).
- S5  a count of previous S1,S2,S3 records, count stored in 16 bit address field + checksum.
- S7,S8,S9  optional, program start address, just an address checksum fields, no data.

Format binary ELF [-fE]
----

A versatile and extendable format, but also more complex. Designed for 32 bit machines.

For CPU architectures that use less than 32 bits, the 32 bit spec is used, but all the high bits of addresses are set to 0 and ignored.
### Main Header
<table>
<tr><th>Offset (hex)</th><th>Length (bytes)</th><th>Field</th><th>Description</th></tr>
<tr><td>00</td><td>4</td><td>Magic</td><td>Big endian byte string that identifies the file (0x7F454C46)</td></tr>
<tr><td>04</td><td>1</td><td>Endianness</td><td>Either 1 - little endian or  2 - big endian</td></tr>
<tr><td>05</td><td>1</td><td>Wordsize</td><td>This field defines the address word size. For processors with addresses less than 32 bits: a 32 bit option is used with extra bits set to 0.<div>
It also defines the size of individual words stored at each address. Values are as follows:</div>
<table>
<tr><th>Value</th><th>Address (bits)</th><th>addrsize</th></tr>
<tr><td>0x01</td><td>32</td><td>4</td></tr>
<tr><td>0x02</td><td>64</td><td>8</td></tr>
</table>
</td></tr>
<tr><td>06</td><td>1</td><td>Version</td><td>Normally set to 0x01</td></tr>
<tr><td>07</td><td>1</td><td>osabitype</td><td>user definable OS type, set to 0x5E by default.</td></tr>
<tr><td>08</td><td>1</td><td>osabiver</td><td>user definable, set to 0x00 by default.</td></tr>
<tr><td>09</td><td>7</td><td>padding</td><td>unused, set to 0</td></tr>
<tr><th colspan="4">Values below are stored in the endianness of the target machine.</th></tr>
<tr><td>10</td><td>2</td><td>exetype</td>
<td>Executable type
<table>
<tr><th>Value</th><th>Meaning</th></tr>
<tr><td>0</td><td>No type</td></tr>
<tr><td>1</td><td>Relocatable</td></tr>
<tr><td>2</td><td>Executable (default)</td></tr>
<tr><td>3</td><td>Shared object</td></tr>
<tr><td>4</td><td>Core</td></tr>
</table>
</td></tr>
<tr><td>12</td><td>2</td><td>arch</td>
<td>Architecture/CPU type. WaveAsm will support the following values for arch:
<table>
<tr><th>Value (dec)</th><th>CPU</th></tr>
<tr><td>0</td><td>None</td></tr>
<tr><td>3</td><td>80386 or x86</td></tr>
<tr><td>4</td><td>68000</td></tr>
<tr><td>13</td><td>65816</td></tr>
<tr><td>20</td><td>PowerPC</td></tr>
<tr><td>32</td><td><b>TR3200</b></td></tr>
<tr><td>40</td><td>ARM32</td></tr>
<tr><td>70</td><td>68HC11</td></tr>
<tr><td>128</td><td><b>DCPU-16</b></td></tr>
<tr><td>155</td><td>AKI24</td></tr>
<tr><td>262</td><td>CAK31135</td></tr>
</table>
</td></tr>
<tr><td>14</td><td>4</td><td>eversion</td><td>default 0x00000001</td></tr>
<tr><td>18</td><td>n=addrsize (see above)</td><td>entry</td><td>beginning address of program execution</td></tr>
<tr><td>18+n</td><td>n</td><td>phstart</td><td>absolute file offset to start of program header</td></tr>
<tr><td>18+2n</td><td>n</td><td>shstart</td><td>absolute file offset to start of sections header</td></tr>
<tr><td>18+3n</td><td>4</td><td>flags</td><td>unused</td></tr>
<tr><td>1C+3n</td><td>2</td><td>headersz</td><td>size of this header: dec 40+3n; hex 0x28+3n</td></tr>
<tr><td>1E+3n</td><td>2</td><td>phsz</td><td>size of program header</td></tr>
<tr><td>20+3n</td><td>2</td><td>phct</td><td>entries in program header</td></tr>
<tr><td>22+3n</td><td>2</td><td>shsz</td><td>size of section header</td></tr>
<tr><td>24+3n</td><td>2</td><td>shct</td><td>entries in section header</td></tr>
<tr><td>26+3n</td><td>2</td><td>shsnindx</td><td>index of sections entry with section names.</td></tr>
</table>
### Program Header

The program header consists of an array of entries. The count entries and starting file offset are in the main header. Each entry in the program header table optionally points to a segment in the file with data.
<table>
<tr><th>Offset (hex)</th><th>Length (bytes)</th><th>Field</th><th>Description</th></tr>
<tr><td>00</td><td>4</td><td>type</td><td>
<table>
<tr><th>Value</th><th>Meaning / Data pointed to</th></tr>
<tr><td>0x00</td><td>Null entry - ignore this entry</td></tr>
<tr><td>0x01</td><td>Loadable data (program code/data)</td></tr>
<tr><td>0x02</td><td>Dynamic link info</td></tr>
<tr><td>0x03</td><td>Interpreter name as Null terminated string</td></tr>
<tr><td>0x04</td><td>A Note table</td></tr>
</table>
</td></tr>
<tr><td>04</td><td>4</td><td>offset</td><td>file offset that segment starts at</td></tr>
<tr><td>08</td><td>4</td><td>vaddr</td><td>Virtual memory address segment should be loaded at</td></tr>
<tr><td>0C</td><td>4</td><td>paddr</td><td>Physical memory address segment should be loaded at. <div>(if applicable)</div></td></tr>
<tr><td>10</td><td>4</td><td>filesz</td><td>Size of segment in file</td></tr>
<tr><td>14</td><td>4</td><td>memsz</td><td>amount of memory to be allocated for segment</td></tr>
<tr><td>18</td><td>4</td><td>flags</td><td>Bit mask of options
<table>
<tr><th>Bit</th><th>Meaning</th></tr>
<tr><td>1</td><td>If set: allow Execute</td></tr>
<tr><td>2</td><td>If set: allow Write</td></tr>
<tr><td>3</td><td>If set: allow Read</td></tr>
<tr><td>21-28</td><td>OS specific</td></tr>
<tr><td>29-32</td><td>CPU specific</td></tr>
</table>
</td></tr>
<tr><td>1C</td><td>4</td><td>align</td><td>The alignment that should be used for the segment. <div>A value of 0 or 1 means no alignment, other values must be a power of 2</div></td></tr>
</table>

### File structure

The typical ELF file generated by WaveAsm consists (minimally) of the main header, followed a two or more entries program header table. A 2 entry section table (the null section and string table). Then program segment(s).

