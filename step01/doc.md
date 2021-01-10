### step-01

Now that we have a machine code monitor, we can
really start typing in our first compiler. It is
called HEX0 and able to compile itself.

A typical line looks like:
```
48C7C2 C0010000     ; LOADI32_RDX %448   # Prepare file as RWX for owner only (700 in octal)
```
We call this format HEX0.
We start with the real CPU-instructions ("opcodes"),
followed by comments: the assembler-symbols ("mnemonics")
and a longer description for the big picture.

Only the opcodes in each line are converted to bytes
and written to a file, which is marked as executable.

The program understand 2 arguments:
Source- and destination filename.

## Details

The commented sourcecode consists of 256 lines of commented
machine code. The resulting binary has a size of 431 bytes
on CPU-architecture x86_64.

```
7f 45 4c 46 02 01 01 03 00 00 00 00 00 00 00 00 02 00 3e 00 01 00 00 00 78 00 60 00 00 
00 00 00 40 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 40 00 38 00 01 00 
00 00 00 00 00 00 01 00 00 00 07 00 00 00 00 00 00 00 00 00 00 00 00 00 60 00 00 00 00 
00 00 00 60 00 00 00 00 00 af 01 00 00 00 00 00 00 af 01 00 00 00 00 00 00 01 00 00 00 
00 00 00 00 58 5f 5f 48 c7 c6 00 00 00 00 48 c7 c0 02 00 00 00 0f 05 49 89 c1 5f 48 c7 
c6 41 02 00 00 48 c7 c2 c0 01 00 00 48 c7 c0 02 00 00 00 0f 05 49 89 c2 49 c7 c7 ff ff 
ff ff 49 c7 c6 00 00 00 00 e8 c4 00 00 00 e8 34 00 00 00 48 83 f8 00 7c f0 49 83 ff 00 
7d 0c 49 89 c6 49 c7 c7 00 00 00 00 eb de 49 c1 e6 04 4c 01 f0 88 04 25 7c 01 60 00 49 
c7 c7 ff ff ff ff e8 6e 00 00 00 eb c2 48 83 f8 23 74 2c 48 83 f8 3b 74 26 48 83 f8 30 
7c 42 48 83 f8 3a 7c 2d 48 83 f8 41 7c 36 48 83 f8 47 7c 2b 48 83 f8 61 7c 2a 48 83 f8 
67 7c 1a eb 22 e8 54 00 00 00 48 83 f8 0a 75 f5 48 c7 c0 ff ff ff ff c3 48 83 e8 30 c3 
48 83 e8 57 c3 48 83 e8 37 c3 48 c7 c0 ff ff ff ff c3 48 c7 c7 00 00 00 00 48 c7 c0 3c 
00 00 00 0f 05 48 c7 c2 01 00 00 00 48 c7 c6 7c 01 60 00 4c 89 d7 48 c7 c0 01 00 00 00 
0f 05 c3 00 00 00 00 48 c7 c2 01 00 00 00 48 c7 c6 ab 01 60 00 4c 89 cf 48 c7 c0 00 00 
00 00 0f 05 48 85 c0 74 b2 8a 04 25 ab 01 60 00 48 0f b6 c0 c3 00 00 00 00
```

FILE="hex0-seed.aarch64.hex0"	# aarch64
FILE="hex0-seed.amd64.hex0"	# x86_64
FILE="hex0-seed.x86.hex0"	# i386

