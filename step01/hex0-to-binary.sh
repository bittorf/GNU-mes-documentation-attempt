#!/bin/sh
# this POSIX shell script only uses builtin-commands
# another approach is: sed 's/[;#].*$//g' file.hex0 | xxd -r -p >hex0.bin

FILE_SRC="$1"		# e.g. any *.hex0 source
FILE_DST="$2"		# e.g. the resulting hex0.bin

while read LINE; do
	for WORD in $LINE; do
		case "$WORD" in
			[a-fA-F0-9][a-fA-F0-9]*) ;;
			*) break ;;			# abort line parsing
		esac

		while [ -n "$WORD" ]; do
			CHAR1="$( printf '%.1s' "$WORD" )"	# 1st char get
			WORD="${WORD#?}"			# 1st char remove
			CHAR2="$( printf '%.1s' "$WORD" )"	# 1st char get
			WORD="${WORD#?}"			# 1st char remove
			HEX="${CHAR1}${CHAR2}"

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
