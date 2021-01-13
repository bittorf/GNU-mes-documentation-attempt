#!/bin/sh

MES_PWD="$( pwd )"		# original repo

TMPDIR="$( mktemp -d )" || exit
cd "$TMPDIR" || exit

git clone https://github.com/bittorf/kritis-linux.git
cd * || exit

TMP1="$( mktemp )" || exit
TMP2="$( mktemp )" || exit

KEEP='/bin/busybox /bin/sh /bin/cat'
echo "[OK] INITRD_DIR_ADD='$MES_PWD' KEEP_LIST='$KEEP' ./minilinux.sh latest"

INITRD_DIR_ADD="$MES_PWD" KEEP_LIST="$KEEP" ./minilinux.sh latest >"$TMP1" 2>"$TMP2" || {
	RC="$?"
	cat "$TMP1" "$TMP2"
	exit "$RC"
}

ls -l minilinux/builds/linux/arch/x86/boot/bzImage	# kernel.bin
ls -l minilinux/builds/initramfs.cpio.xz.xz 		# initrd.xz
apt-cache search qemu

( sleep 300 && killall qemu-system-x86_64 ) &
minilinux/builds/linux/run.sh

exit $?
