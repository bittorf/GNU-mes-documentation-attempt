#!/bin/sh

# https://github.com/bittorf/kritis-linux
git clone --depth 1 https://github.com/bittorf/kritis-linux.git

kritis-linux/ci_helper.sh \
	--arch 'x86_64' \
	--kernel "latest" \
	--features 'busybox' \
	--keep '/bin/busybox /bin/sh /bin/cat' \
	--diradd "$( pwd )" \
	--myinit "run-amd64.sh" \
	--maxwait "200" \
	--pattern "In QEMU-mode you can now explore the system"
