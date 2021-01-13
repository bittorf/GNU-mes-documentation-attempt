#!/bin/sh
# this POSIX shell script only uses builtin-commands
# another approach is: sed 's/[;#].*$//g' file.hex0 | xxd -r -p >hex0.bin

FILE_SRC="$1"		# e.g. any *.hex0 source
FILE_DST="$2"		# e.g. the resulting hex0.bin
#set -x
while read LINE; do
	for WORD in $LINE; do
		case "$WORD" in
			[a-fA-F0-9][a-fA-F0-9]*) ;;
			*) break ;;			# abort line parsing
		esac

		while [ -n "$WORD" ]; do
logger -s "SHL: '$SHELL'"
logger -s "Trk: '${WORD%${WORD#??}}'"
logger -s "Tr2: '${WORD#?}'"
			HEX="${WORD#??}"
logger -s "HEX: '$HEX'"
			HEX="${HEX%$HEX}"		# read first two chars
logger -s "HEx: '$HEX'"
			WORD="${WORD#??}"		# remove first two chars
logger -s "WOd: '$WORD'"
exit
			case "$HEX" in
				[a-fA-F0-9][a-fA-F0-9])
					OCTAL="$( printf "%o" "0x$HEX" )"
					eval printf "\\\\$OCTAL"
				;;
				*)
					break		# abort line parsing
				;;
			esac
		done
	done
done <"$FILE_SRC" >"$FILE_DST"
