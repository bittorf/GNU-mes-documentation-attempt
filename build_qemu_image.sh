#!/bin/sh

git clone --depth 1 https://github.com/bittorf/kritis-linux.git
kritis-linux/ci_helper.sh \
	--keep '/bin/busybox /bin/sh /bin/cat' \
	--diradd "$( pwd )" \
	--pattern "In QEMU-mode you can now explore the system" \
	--maxwait "600" \
	--linux "latest"
