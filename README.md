# Another hobbyiest attempt at OSDev

This repository contains the source code for a hobbyist 32-bit monolithic kernel inspired by several UNIX like operating systems. Nothing too significant this project at this time.

![screenshot](https://i.imgur.com/SpDeU6W.png "Screenshot")

### Goals
My main goal with this project is simply to develop self-hosted UNIX clone for my own benefit. 
I would like to minimize the use of third party software as much as possible, however, there are a few exceptions, most notability the use of newlib for the C library as well as GRUB for the bootloader.

### Building
Compiling this requires a custom GCC capable of targeting this kernel. Patching GCC and dealing with GNU autocruft is a long and tedious process. I've done my best to attempt to automate as much of this as possible. I first recommend this be built from a Centos 7 environment. You can use something like `docker` or `podman` on GNU/Linux systems or WSL on Windows (See [CentWSL](https://github.com/yuk7/CentWSL "CentWSL") to get a Centos 7 image for WSL).

Once in a Centos 7 environment, install the pre-requisites by running:
```
yum install -y gcc gcc-c++ vim git wget m4 perl-Data-Dumper patch gmp-devel mpfr-devel libmpc-devel make nasm grub2 xorriso && yum update -y
```
Then, to build, run:

```
git submodule init
git submodule update
make toolchain kernel userland-libraries xtc iso
```

An ISO image should be created in `build/os.iso`

### Directory Structure
|Path|Description |
| ------ | ------|
|/bin|Core userland utilities found in /bin|
|/build|Houses toolchain and temporary files required for build process|
|/sbin|Source for userland binaries installed in /sbin|
|/sys|Source for the kernel itself|
|/sys/dev|Kernel device implementations|
|/sys/fs|Kernel filesystem implementations|
|/sys/kern|Core kernel components|
|/sys/net|Kernel network protocol implementations|
|/sys/i686|i686 architecture dependent source|
|/sys/i686/dev|i686 device implementations|
|/sys/i686/kern|i686 kernel components|
|/usr.lib|Usermode libraries|
|/usr.libexec|Source for userland binaries installed in /usr/libexec|

