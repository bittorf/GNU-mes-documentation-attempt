[travis]:       https://travis-ci.org/bittorf/GNU-mes-documentation-attempt
[travisGFX]:    https://travis-ci.org/bittorf/GNU-mes-documentation-attempt.png

### goal

A try to understand the GNU mes bootstrap-a-compiler project

## method one

Clone this repository and execute `./run-amd64.sh`:
```
# git clone --depth 1 https://github.com/bittorf/GNU-mes-documentation-attempt.git
# cd GNU-mes-documentation-attempt
# ./run-amd64.sh
or
# AUTO=true ./run-amd64.sh
```

## method two

Clone this repository and start QEMU with the image:
```
# git clone --depth 1 https://github.com/bittorf/GNU-mes-documentation-attempt.git
# cd GNU-mes-documentation-attempt
# qemu-system-x86_64 -kernel kernel.bin -initrd initrd.xz -nographic -append "console=ttyS0"
or
# qemu-system-x86_64 -enable-kvm -cpu host -kernel kernel.bin -initrd initrd.xz -nographic -append "console=ttyS0"
```
This image has a linux-kernel (allno-config), a statically compiled busybox,
where only 'sh' (the shell) and 'cat' is enabled and a ramdisk
with this repository. After running the steps, you can call
not enabled (symlinked) commands with e.g. `busybox ls -l /tmp`.

## TODO

* document all steps
* add build instructions for qemu-image
* autoupdate all files from scattered repos
* build mes (the c-compiler written in scheme)
* build mes-cc (the c-compiler which can build tcc)
* build tcc-pre
* build tcc-real
* build gcc
* build toolchain
* autobuild kaem-script from doc
* include 'kaem' and 'catm'
* build HTML-page
* add all architectures (i386, aarch64, arm, knight)
* provide minimal architecture agnostic .tgz files for full run
* measure SLOC for initial bootstrap code: MES vs. ASMC vs. XY

