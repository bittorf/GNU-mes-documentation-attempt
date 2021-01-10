### goal

A try to understand the GNU mes bootstrap-a-compiler project

## method one

Clone this repository and execute `./run-amd64.sh`:
```
# git clone https://github.com/bittorf/GNU-mes-documentation-attempt.git
# cd GNU-mes-documentation-attempt
# ./run-amd64.sh
```

## method two

Clone this repository and start QEMU with the image:
```
# git clone https://github.com/bittorf/GNU-mes-documentation-attempt.git
# cd GNU-mes-documentation-attempt
# qemu-system-x86_64 -kernel kernel.bin -initrd initramfs.cpio.gz -nographic -append "console=ttyS0"
```
This image has a allno-config linux-kernel, a statically compiled busybox,
where only 'sh' (the shell) and 'cat' is enabled and a ramdisk
with this repository.

## TODO

* document all steps
* add build instructions for qemu-image
* add architecture i386 + aarch64
* build mes (the c-compiler written in scheme)
* build mes-cc (the c-compiler which can build tcc)
* build tcc-mini
* build tcc-real
* build gcc
* build toolchain
* include 'kaem' and 'catm'

