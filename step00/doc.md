### step-00

We start with nothing and need to have a way of typing
in our first program. A [machine code monitor](https://en.wikipedia.org/wiki/Machine_code_monitor)
will solve this problem: Now we can enter hex-values,
which are converted to bytes and written to a file:

Our first binary.

Unfurtunately this program has a chicken-and-egg problem:
Unlike early home computers, we have no machine code monitor
running yet, so we must must provide it as binary or
jump direclty to step-01...

## Details

The commented sourcecode consists of 69 short lines of
machine code and must be typed in. The resulting binary
has a size of 342 bytes on CPU-architecture x86_64.

TODO: add 64 bytes ELF-header
TODO: add copy/paste mode for getting the feeling

These values must be typed in:
```
e0 00 2d 2f 00 0f e0 00 2d 2b 11 01 01 10 0f b8 42 10 00 01 09 00 04 0b 42 10 00 01 0d
00 00 21 42 10 01 00 e0 00 a0 30 00 0d e0 00 2d 20 00 0a 42 10 02 00 e0 00 a0 30 00 04
3c 00 01 04 e0 00 2c c0 00 fe 09 00 04 1b 42 10 02 00 e0 00 2d 0d 00 36 e0 00 2c c0 ff
c4 e0 00 2c ac 00 0c 05 02 02 0f 0d 00 00 3c 3c 00 ff b2 e0 00 2d 52 00 04 05 02 00 0f
05 00 00 02 0d 00 00 2c 01 10 1f b8 42 10 02 00 3c 00 ff 94 e1 00 1f e0 00 23 e0 00 2c
5e 00 7e e1 00 1f e0 00 3b e0 00 2c 5e 00 72 e1 00 1f e0 00 30 e0 00 2c 8e 00 5e e1 00
1f e0 00 39 e0 00 2c 7e 00 34 e1 00 1f e0 00 41 e0 00 2c 8e 00 46 e1 00 1f e0 00 46 e0
00 2c 7e 00 30 e1 00 1f e0 00 61 e0 00 2c 8e 00 2e e1 00 1f e0 00 66 e0 00 2c 7e 00 0e
3c 00 00 1e e1 00 11 00 00 30 0d 01 00 1d e1 00 11 00 00 57 0d 01 00 1d e1 00 11 00 00
37 0d 01 00 1d 0d 00 00 30 0d 01 00 1d 0d 00 00 21 42 10 01 00 e0 00 a0 30 00 0d e0 00
2d 20 00 0a 42 10 02 00 e1 00 1f e0 00 0a 09 00 04 1b 42 10 02 00 e0 00 2c 6e ff d4 3c
00 ff c8 01 10 0f b8 42 10 00 02 09 00 04 0b 42 10 00 02 ff ff ff ff
```

For getting the idea, how this code works, it is
included for documentation as C- and assembler source.

REPO="https://github.com/oriansj/stage0.git"	# master
FILE="stage0/stage0_monitor.s" 			# x86_64
FILE="stage0/stage0_monitor.hex0"		# x86_64
FILE="High_level_prototypes/stage0_monitor.c"

