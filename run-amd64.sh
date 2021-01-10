#!/bin/sh
#
# sorry, this script needs still a lot of
# cleanup, please ignore all the comment for now
#
# https://www.gnu.org/software/mes/
# https://www.gnu.org/software/mes/manual/mes.html#Invoking-mescc
# https://bootstrappable.org/projects/mes.html
# https://bootstrapping.miraheze.org/wiki/Stage0#M2-Planet_+_mescc-tools
# https://github.com/oriansj/stage0
# https://github.com/oriansj/stage0/blob/master/README.org
# https://github.com/oriansj/mescc-tools-seed
# https://github.com/oriansj/mes-m2
# https://github.com/oriansj/talk-notes/blob/master/talks.org
# https://github.com/oriansj/blynn-compiler
# https://www.gnu.org/software/mes/manual/mes.html
# https://www.gnu.org/software/mes/manual/html_node/Full-Source-Bootstrap.html
# http://logs.guix.gnu.org/bootstrappable/2020-09-12.log -> gforce_de1977 + AS196714hotmilf
# https://gitlab.com/janneke/mes/-/tree/master
# https://reproducible-builds.org/news/2019/12/21/reproducible-bootstrap-of-mes-c-compiler/
# https://gitlab.com/giomasce/nbs
# https://gitlab.com/giomasce/asmc/-/blob/master/.travis.yml
# https://github.com/fosslinux/live-bootstrap/
# https://joyofsource.com/projects/bootstrappable-tcc.html
# https://stackoverflow.com/questions/22376285/is-it-possible-to-assemble-and-run-raw-cpu-instructions-using-as
#
# https://www.freelists.org/post/bootstrappable/wipfullsourcebootstrap-from-a-357byte-hex0-to-hello
# bootstrap to tcc: https://bpa.st/53FQ
# mes -> mescc -> tcc -> GCC 2.95.0 -> GCC 4.7.5 -> GCC10?
#
# ( https://diveinto.html5doctor.com )
#
# LAP: ( cd .. && tar czf gnu-mes-documentation-attempt.tgz gnu-mes-documentation-attempt )
#      scp ../gnu-mes-documentation-attempt.tgz 10.63.22.100:software/
# RYZ: ( cd software/ && tar xzf gnu-mes-documentation-attempt.tgz )
#      KEEP='/bin/busybox /bin/sh /bin/cat'
#      DIR='/home/bastian/software/gnu-mes-documentation-attempt'
#
#      INITRD_DIR_ADD="$DIR" KEEP_LIST="$KEEP" ./minilinux.sh 32
# LAP: scp 10.63.22.100:/home/bastian/software/minilinux/minilinux/builds/linux/arch/x86/boot/bzImage kernel.bin
#      scp 10.63.22.100:/home/bastian/software/minilinux/minilinux/builds/initramfs.cpio.gz initramfs.cpio.gz
#      git add kernel.bin initramfs.cpio.gz && git commit -m "update qemu image"


# "with M2-Planet being the simplest to port to your architecture, MesCC is in Scheme"
# "good them MesCC would be very fun for you."
# "There are a few minor things you might want to know about when generating binaries with MesCC. The C code gets converted to M1-macro assembly and then linked with hex2"


# "ullbeking: hex1 is just hex0 with single character labels ;a and one size relative pointers %a to save the work of having to manually calculate jump offsets"

# "hex2 extends hex1 with long label names, multiple relative (!8bit, @16bit ~architecture specific and %32bit) and absolute ($16bit and &32bit) pointer sizes"


# "I feel like a lot of compiler theory is counterproductive because it makes the problem seem harder than it is"
# "Jack Crenshaw's "Let's Build a Compiler" is one that doesn't, but I haven't actually read it"

### dependencies:
# (mktemp) -> include dir /tmp and try to write a testfile
# sh/ash/dash -> later kaem
# cat -> later mcat

show_doc()
{
	printf '\033c\e[3J'	# clear screen

	# output file until 'details' section:
	while read -r LINE; do
		case "$LINE" in '## Details') break ;; *) printf '%s\n' "$LINE" ;; esac
	done <"$1"

	printf '%s' '<press enter to continue>'
	read NOP

	# output file starting at 'details' section:
	PARSE=
	while read -r LINE; do
		case "$LINE" in '## Details') PARSE=yes ;; esac
		case "$PARSE" in yes) printf '%s\n' "$LINE" ;; esac
	done <"$1"

	case "$PARSE" in '') return ;; esac

	printf '%s' '<press enter to continue>'
	read NOP
}

CHMOD="$( command -v chmod || echo false )"
MKTEMP="$( command -v mktemp || echo false )"
TMPDIR="$( $MKTEMP -d || echo /tmp )"		# find /tmp -type d -name 'tmp.*' -exec rm -fR {} \; 2>/dev/null

# overview:
show_doc 'doc.md'

# machine monitor: type in hex0 source into memory/file
show_doc 'step00/doc.md'

# produce HEX0
show_doc 'step01/doc.md'
COMPILER='step01/hex0-to-binary.sh'
SRC="step01/hex0-seed.amd64.hex0"
DST="$TMPDIR/hex0.bin"
$COMPILER "$SRC" "$DST" || exit
$CHMOD +x "$DST"

echo "### step02 | produce 'HEX1'"
COMPILER_HEX0="$DST"
SRC='step02/hex1_AMD64.hex0'
DST="$TMPDIR/hex1.bin"
$COMPILER_HEX0 "$SRC" "$DST" || exit

echo "### step03 | produce 'HEX2'"
COMPILER_HEX1="$DST"
SRC='step03/hex2_AMD64.hex1'
DST="$TMPDIR/hex2.bin"
$COMPILER_HEX1 "$SRC" "$DST" || exit

echo "### step04 | produce 'M0'"
COMPILER_HEX2="$DST"
SRC="$TMPDIR/step04-elf-m0.hex2"
SRC1='step04/ELF-amd64.hex2'
SRC2='step04/M0_AMD64.hex2'
cat "$SRC1" "$SRC2" >"$SRC"
DST="$TMPDIR/M0.bin"
$COMPILER_HEX2 "$SRC" "$DST" || exit

echo "### step05 | produce 'CC'"
COMPILER_M0="$DST"
SRC2='step05/cc_amd64.M1'
DST1="$TMPDIR/step05-cc.hex2"
$COMPILER_M0 "$SRC2" "$DST1"
#
SRC1='step05/ELF-amd64.hex2'
SRC="$TMPDIR/step05-cc-all.hex2"
cat "$SRC1" "$DST1" >"$SRC"
#
DST="$TMPDIR/step05-cc.bin"
$COMPILER_HEX2 "$SRC" "$DST" || exit

echo "### step06 | produce ???"
COMPILER_CC="$DST"
SRC="$TMPDIR/step06-all.c"
DST="$TMPDIR/M2.M1"
cat 'step06/amd64/'* >"$SRC"
$COMPILER_CC "$SRC" "$DST" || exit

echo "### step07 | produce 'M2.hex2'"
SRC1='step07/amd64_defs.M1'
SRC2='step07/libc-core.M1'
SRC3="$TMPDIR/M2.M1"
SRC="$TMPDIR/step07-all.M1"
cat "$SRC1" "$SRC2" "$SRC3" >"$SRC"
DST="$TMPDIR/M2.hex2"
$COMPILER_M0 "$SRC" "$DST" || exit

echo "### step08 | produce 'M2'"
SRC1='step08/ELF-amd64.hex2'
SRC2="$DST"
SRC="$TMPDIR/M2-full.hex2"
cat "$SRC1" "$SRC2" >"$SRC"
#
DST="$TMPDIR/M2.bin"
$COMPILER_HEX2 "$SRC" "$DST" || exit
COMPILER_M2="$DST"

echo "### step09 | produce 'blood-elf.M1'"
DST="$TMPDIR/blood-elf.M1"
ARGS="$( for SRC in step09/*; do printf '%s ' "-f $SRC"; done )"
# blood-elf.c calloc.c exit.c file.c file_print.c in_set.c malloc.c match.c numerate_number.c require.c
$COMPILER_M2 --architecture amd64 $ARGS -o "$DST" || exit

echo "### step10 | produce 'M2' ???"
SRC1='step07/amd64_defs.M1'
SRC2='step07/libc-core.M1'
SRC3="$DST"
SRC="$TMPDIR/step10-all.M1"
cat "$SRC1" "$SRC2" "$SRC3" >"$SRC"
DST="$TMPDIR/step10-result.hex2"
$COMPILER_M0 "$SRC" "$DST" || exit

echo "### step11 | produce 'blood-elf'"
SRC1='step08/ELF-amd64.hex2'
SRC2="$DST"
SRC="$TMPDIR/step11-all.hex2"
cat "$SRC1" "$SRC2" >"$SRC"
DST="$TMPDIR/blood-elf-0.bin"
$COMPILER_HEX2 "$SRC" "$DST" || exit

echo "### step12 | produce 'M1-macro.M1'"
COMPILER_BLOODELF="$DST"
ARGS="$( for SRC in step12/*; do printf '%s ' "-f $SRC"; done )"
# calloc.c exit.c file.c file_print.c in_set.c M1-macro.c malloc.c match.c numerate_number.c require.c string.c
DST="$TMPDIR/M1-macro.M1"
$COMPILER_M2 --architecture amd64 $ARGS --debug -o "$DST" || exit

echo "### step13 | produce 'M1-macro-footer.M1'"
SRC="$DST"
DST="$TMPDIR/M1-macro-footer.M1"
$COMPILER_BLOODELF --64 -f "$SRC" -o "$DST" || exit

echo "### step14 | produce 'M1-macro-full.hex2'"
SRC1='step07/amd64_defs.M1'
SRC2='step07/libc-core.M1'
SRC3="$TMPDIR/M1-macro.M1"
SRC4="$TMPDIR/M1-macro-footer.M1"
SRC="$TMPDIR/M1-macro-full.M1"
cat "$SRC1" "$SRC2" "$SRC3" "$SRC4" >"$SRC"
#
DST="$TMPDIR/M1-macro-full.hex2"
$COMPILER_M0 "$SRC" "$DST" || exit

echo "### step15 | produce 'M1'"
SRC1='step08/ELF-amd64.hex2'
SRC2="$DST"
SRC="$TMPDIR/M1-macro-full_with_header.hex2"
cat "$SRC1" "$SRC2" >"$SRC"
#
DST="$TMPDIR/M1.bin"
$COMPILER_HEX2 "$SRC" "$DST" || exit
COMPILER_M1="$DST"

echo "### step16 | produce 'HEX3'"
DST="$TMPDIR/hex2_linker.M1"
ARGS="$( for SRC in step16/*; do printf '%s ' "-f $SRC"; done )"
# calloc.c exit.c file.c file_print.c hex2_linker.c in_set.c malloc.c match.c numerate_number.c require.c stat.c
$COMPILER_M2 --architecture amd64 $ARGS --debug -o "$DST"
SRC="$DST"
DST="$TMPDIR/hex2_linker-footer.M1"
$COMPILER_BLOODELF --64 -f "$SRC" -o "$DST" || exit
#
SRC1='step07/amd64_defs.M1'
SRC2='step07/libc-core.M1'
SRC3="$TMPDIR/hex2_linker.M1"
SRC4="$DST"
ARGS="-f $SRC1 -f $SRC2 -f $SRC3 -f $SRC4"
DST="$TMPDIR/hex3-pre.hex2"
$COMPILER_M1 $ARGS --LittleEndian --architecture amd64 -o "$DST" || exit
SRC1='step08/ELF-amd64.hex2'
SRC2="$DST"
SRC="$TMPDIR/hex3-fill.hex2"
cat "$SRC1" "$SRC2" >"$SRC"
DST="$TMPDIR/hex3.bin"
$COMPILER_HEX2 "$SRC" "$DST" || exit
COMPILER_HEX3="$DST"

echo "### step17 | produce 'blood-elf-full.bin'"
ARGS="$( for SRC in step17/*; do printf '%s ' "-f $SRC"; done )"
# blood-elf.c calloc.c exit.c file.c file_print.c in_set.c malloc.c match.c numerate_number.c require.c
DST="$TMPDIR/blood-elf.M1"
$COMPILER_M2 --architecture amd64 $ARGS --debug -o "$DST" || exit
#
SRC="$DST"
DST="$TMPDIR/blood-elf-footer.M1"
$COMPILER_BLOODELF --64 -f "$SRC" -o "$DST"
#
SRC1='step07/amd64_defs.M1'
SRC2='step07/libc-core.M1'
SRC3="$SRC"
SRC4="$DST"
ARGS="-f $SRC1 -f $SRC2 -f $SRC3 -f $SRC4"
DST="$TMPDIR/blood-elf-full.hex2"
$COMPILER_M1 $ARGS --LittleEndian --architecture amd64 -o "$DST" || exit
#
SRC1='step04/ELF-amd64.hex2'
SRC2="$DST"
DST="$TMPDIR/blood-elf-full.bin"
ARGS="--LittleEndian --architecture amd64 --BaseAddress 0x00600000"
$COMPILER_HEX3 -f $SRC1 -f $SRC2 $ARGS -o "$DST" --exec_enable ||exit
COMPILER_BLOODELF_FULL="$DST"

echo "### step18 | produce 'M2-planet' - OriansJ:M2-Planet code is standard C code"
ARGS="$( for SRC in step18/*; do printf '%s ' "-f $SRC"; done )"
# calloc.c cc.c cc_core.c cc_globals.c cc.h cc_reader.c cc_strings.c cc_types.c
# exit.c file.c file_print.c fixup.c in_set.c malloc.c match.c number_pack.c
# numerate_number.c require.c string.c
DST="$TMPDIR/better-M2.M1"
$COMPILER_M2 --architecture amd64 $ARGS--debug -o "$DST"
SRC="$DST"
DST="$TMPDIR/M2-footer.M1"
$COMPILER_BLOODELF_FULL --64 -f "$SRC" -o "$DST"
SRC1='step07/amd64_defs.M1'
SRC2='step07/libc-core.M1'
SRC3="$SRC"
SRC4="$DST"
ARGS="-f $SRC1 -f $SRC2 -f $SRC3 -f $SRC4"
DST="$TMPDIR/M2-planet.hex2"
$COMPILER_M1 $ARGS --LittleEndian --architecture amd64 -o "$DST" || exit
SRC1='step04/ELF-amd64.hex2'
SRC2="$DST"
DST="$TMPDIR/M2-Planet.bin"
ARGS="--LittleEndian --architecture amd64 --BaseAddress 0x00600000"
$COMPILER_HEX3 -f "$SRC1" -f "$SRC2" $ARGS -o "$DST" --exec_enable || exit
COMPILER_M2PLANET="$DST"

echo "### step19 | produce 'mes-m2'"
ARGS="$( for SRC in step19/*; do printf '%s ' "-f $SRC"; done )"
# calloc.c exit.c file.c file_print.c in_set.c malloc.c match.c
# mes_builtins.c mes.c mes_cell.c mes_eval.c mes.h mes_init.c mes_keyword.c mes_list.c mes_macro.c
# mes_posix.c mes_print.c mes_read.c mes_record.c mes_string.c mes_tokenize.c mes_vector.c 
# numerate_number.c
DST="$TMPDIR/mes.M1"
$COMPILER_M2PLANET --debug --architecture amd64 $ARGS -o "$DST" || exit
#
SRC="$DST"
DST="$TMPDIR/mes-footer.M1"
$COMPILER_BLOODELF_FULL --64 -f "$SRC" -o "$DST" || exit
#
SRC1='step07/amd64_defs.M1'
SRC2='step07/libc-core.M1'
SRC3="$SRC"
SRC4="$DST"
DST="$TMPDIR/mes.hex2"
ARGS="-f $SRC1 -f $SRC2 -f $SRC3 -f $SRC4"
$COMPILER_M1 $ARGS --LittleEndian --architecture amd64 -o "$DST" || exit
#
SRC1='step04/ELF-amd64.hex2'
SRC2="$DST"
ARGS="--LittleEndian --architecture amd64 --BaseAddress 0x00600000"
DST="$TMPDIR/mes-m2.bin"
$COMPILER_HEX3 -f "$SRC1" -f "$SRC2" $ARGS -o "$DST" --exec_enable || exit
COMPILER_MESM2="$DST"

# MES_CORE=0 /tmp/tmp.3nSjiPTcKm/mes-m2.bin --help
# Usage: /tmp/tmp.3nSjiPTcKm/mes-m2.bin [--boot boot.scm] [-f|--file file.scm] [-h|--help]






echo "### step20 | TODO: produce 'mes' + 'mes-c-library' from 'mescc.scm' via 'mes-m2.bin'"



# bastian@X301:~/software/gnu-mes-documentation-attempt/x/mes-0.22-305-g2ab4c5c67$ ./bin/mes --help
# Usage: mes [OPTION]... [FILE]...
# Scheme interpreter for bootstrapping the GNU system.



# bastian@X301:~/software/gnu-mes-documentation-attempt/x/mes-0.22-305-g2ab4c5c67$ ./pre-inst-env mescc --help
# Usage: mescc [OPTION]... FILE...
# C99 compiler in Scheme for bootstrapping the GNU system.

# MES=guile V=2 ./bootstrap.sh

# 18:08 < janneke> mescc builds the patched tcc initially without -D HAVE_LONG_LONG, without -D HAVE_FLOAT (and some more)

# 18:13 < janneke> gforce_de1977: i guess you know that blood-elfX adds dwarf debug symbols

# stage3/blood-elf_x86.c:486:IIIfile_print("blood-elf 0.1\n(Basically Launches Odd Object Dump ExecutabLe Files\n", stdout);

#18:27 < rain1> > you can build gcc with tinycc with mescc which is a little c compiler and stlib written in scheme,
#                 which can be bootstrapped off M2-Planet which is a scheme interp written in C.. 
#                 that is compiled using a very basic c compiler written in assembly which can be 
#                 assembled by some other tools that are intitally built using just a hex->binary translator
#18:28 < rain1> https://bpa.st/VHAQ building tcc seems to be working! it just installed to a wrong location
#18:29 < janneke> "M2-Planet which is a scheme interp written in C"
#18:29 < janneke> M2-Planet is a transpiler for the M2 language, which closely resembles a subset of C
#
#18:35 < janneke> you could add details, such as that mes is a scheme interpreter and is bootstrapped by m2-planet
#18:35 < rain1> so to correct this, mescc is running in mes scheme interpreter, which is compiled by M2-Planet (the C-subset) ?
#18:35 < janneke> or that building gcc is also not a trivial step


#19:32 < yt_> on the one hand, M2-Planet being a subset of C is great, cause I can write and debug most of a program with gcc and good tool
#             support.  On the other hand, the incompatiblities can bite hard sometimes: "if (!pointer)" silently "miscompiles".

#19:42 < janneke> more debug info for m2-planet or mescc binaries would be great
#19:45 < yt_> jannke: m2-planet is slightly ahead of mescc in that respect, as it at least has ELF symbols
#19:46 < yt_> ^jannke^janneke^
#19:49 < stikonas> yeah, I just had a crash in mes, and can't get backtrace :( (probably something wrong in my build script)
#19:50 < janneke> yt_: huh, what does m2-planet have more than mescc?
#19:50 < janneke> i may have missed something, mescc only has function symbols
#19:51 < yt_> janneke: oh maybe I've missed something in mescc? I thought it didn't generate debug ELF?
#19:52 < janneke> mescc has an (almost) posix command line interface; did you use -g?
#19:54 < janneke> iirc, OriansJ has created blood-elf for mescc, before creating m2-planet anyway, he may have had plans earlier



# transpiler = source-2-source translator


echo "### step21 | TODO: use 'mes' to compile a patched 'tcc'"
# test:
# mes -c '(display "Hello") (newline)'


#12:43 < gforce_d11977> happy winterday to everyone! i watched all the talks and read the slides, but there are still questions: in 
#                       "https://www.gnu.org/software/mes/manual/mes.html#The-Mes-Bootstrap-Process" at "3.2 Invoking mes": is our initial 
#                       'mes' the binary which is produced in "mescc-tools-seed" when running make as "bin/mes-m2" a 233k binary which wants 
#                       to eat an .scm file?
#12:45 -!- stikonas [~gentoo@wesnoth/translator/stikonas] has quit [Remote host closed the connection]
#12:46 -!- stikonas [~gentoo@wesnoth/translator/stikonas] has joined #bootstrappable
#12:49 < janneke> gforce_d11977: not at the moment
#12:49 -!- jas4711 [~smuxi@31-208-42-58.cust.bredband2.com] has joined #bootstrappable
#12:49 < janneke> mescc-tools-seed produces m2-planet, which can be used to build the wip-m2 branch of mes
#12:50 < janneke> and yes, that produces a bin/mes-m2
#12:50 < gforce_d11977> janneke: ok, but this "bin/mes-m2" has not yet the capabilities to compile the .scm file for producing mescc?
#12:52 < janneke> mescc is written in scheme, you can think of it as mes.c (or mes.m2) and mescc.scm
#12:52 < janneke> mes-m2 can run mescc.scm to produce itself (mes) and the mes c library
#12:52 < janneke> these can then build tinycc
#12:54 < gforce_d11977> janneke: sorry, i mixed this up. thanks for explanation, i will try these steps
#12:55 < janneke> have i seen the wip-full-source-bootstrap branch in guix?
#12:55 < janneke> np
#12:56 < janneke> *have you seen...
#12:57 < gforce_d11977> janneke: yes, i have the branch opened here, but ... still dont get it (give me some time...)
#12:57 < gforce_d11977> (dont want to install guix, so i try it manually)
#12:57 < janneke> np, just good to not unknowingly miss info :)



#15:00 < gforce_d11977> janneke: now i'am really lost: i have built bin/mes-m2 - how can i have 'mescc' (which is not build yet) to compile 
#                       'mes'? sorry for dumb questions, all the other 19 steps (from hex0) are clear and working, but now....? bootstrap.sh 
#                       even calls 'perl' 8-)
#15:02 < gforce_d11977> (sorry, configure.sh searches for 'perl')
#15:07 < janneke> perl is for help2man in the full build; that's probably not supported or needed in the bootstrap build
#15:07 < janneke> it's probably nice that configure.sh leaves the same OK state as the development ./configure does
#15:07 < janneke> gforce_d11977: do you have a bin/mes ?
#15:08 < janneke> what doet ./pre-inst-env mescc --help say?
#15:09 < stikonas> I actually get something like "unhandled exception: unbound-variable: (abort)" with mescc from wip-m2 branch (manually 
#                  built with M2-Planet)
#15:09 < stikonas> any ideas?
#15:10 < gforce_d11977> janneke: interesting, it searches for 'nyacc/lex.scm' - i have to install that...
#15:18 < gforce_d11977> janneke: wow, now it says: 'usage: ... \nC99 compiler in Scheme for bootstrapping the GNU system.'
#15:19 < gforce_d11977> janneke: now i really have to re-try what i have done in the last hour and think about it, why it worked 8-)
#15:20 < janneke> nice
#16:26 < janneke> stikonas: you may try MES_DEBUG=2 ... and see if you get more info


# https://bpa.st/GORQ
#
#$ ../mes/bin/mes-mescc
#GNU Mes 0.22
#Copyright (C) 2016,2017,2018,2019 Jan (janneke) Nieuwenhuizen <janneke@gnu.org>
#Copyright (C) 2019 Danny Milosavljevic <dannym@scratchpost.org>
#
#GNU Mes comes with ABSOLUTELY NO WARRANTY; for details type `,show w'.
#This program is free software, and you are welcome to redistribute it
#under certain conditions; type `,show c' for details.
#
#Enter `,help' for help.
#mes> ,help
#[sexp=(unquote help)]
#Help Commands:
#
#  ,expand SEXP         - Expand SEXP
#  ,help                - Show this help
#  ,quit                - Quit this session
#  ,show TOPIC          - Show info on TOPIC [c, w]
#  ,use MODULE          - load MODULE
#mes> 


# OriansJ:also it would be funny to say the secret to bootstrapping GCC was to write a Lisp in Haskell to run a C compiler written in scheme.



ls -l "$DST" || busybox ls -l "$DST"
echo "dir: $TMPDIR | rm -fR $TMPDIR"

# D=$(pwd)	# e.g. in step18
# LIST='...'
# I=0; for F in $LIST; do test -f "$F" || continue; J=$I; test ${#J} -eq 1 && J=0$J; B=$(basename $F); cp -v $F $D/${J}_$B; I=$((I+1)); done
# for F in $( cd step18 && ls -1 | cut -b4- | sort ); do printf '%s ' $F; done
