#!/bin/sh
# this POSIX shell script only uses builtin-commands
# # another approach is: sed 's/[;#].*$//g' file.hex0 | xxd -r -p >hex0.bin

FILE_SRC="$1"		# e.g. any *.hex0 source
FILE_DST="$2"		# e.g. the resulting hex0.bin
DEBUG="$3"		# e.g. <empty> or true

I=0			# counter hex-values
J=0			# counter lines
K=0			# counter values/line
while read -r LINE; do
	for WORD in $LINE; do
		case "$WORD" in
			[a-fA-F0-9][a-fA-F0-9]*) J=$(( J + 1 ));;
			*) break ;;			# abort line parsing
		esac

		while [ -n "$WORD" ]; do
			HEX="${WORD%${WORD#??}}"	# read first two chars
			WORD="${WORD#??}"		# remove first two chars

			case "$HEX" in
				[a-fA-F0-9][a-fA-F0-9])
					I=$(( I + 1 ))
					K=$(( K + 1 ))
					case "$DEBUG" in
						true)
							case "$K" in 30) K=1 && >&2 printf '\n' ;; esac
							>&2 printf '%s ' "$HEX"
						;;
					esac

					OCTAL="$( printf "%o" "0x$HEX" )"
					eval printf "\\\\$OCTAL"
				;;
				*)
					break		# abort line parsing
				;;
			esac
		done
	done
done <"$FILE_SRC" >"$FILE_DST" && printf '%s\n' "$0: [OK] parsed $J lines and wrote $I hex values"
