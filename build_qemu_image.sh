#!/bin/sh

MES_PWD="$( pwd )"		# original repo

TMPDIR="$( mktemp -d )" || exit
cd "$TMPDIR" || exit

git clone https://github.com/bittorf/kritis-linux.git
cd * || exit

KEEP='/bin/busybox /bin/sh /bin/cat'
INITRD_DIR_ADD="$MES_PWD" KEEP_LIST="$KEEP" ./minilinux.sh latest || exit

ls -l minilinux/builds/linux/arch/x86/boot/bzImage	# kernel.bin
ls -l minilinux/builds/initramfs.cpio.xz.xz 		# initrd.xz

