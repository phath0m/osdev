# Another hobbyiest attempt at OSDev

This repository contains the source for my hobbyiest OSDev project that I have been developing off and on since 2018.

![screenshot](https://i.imgur.com/SpDeU6W.png "Screenshot")

### Goals
My main goal with this project is simply to develop self-hosted UNIX clone for my own benefit. 

### Building
Compiling this project requires a custom GCC capable of targeting this kernel.

The `Dockerfile` provided with this repository can be used to create a Centos 7 image
with all of the required development dependencies.

Using this Centos 7 image, you can compile the entire operating system via:

```
git submodule init
git submodule update
make toolchain kernel userland-libraries userland iso
```

An ISO image should be created in `build/os.iso`

### Where's the GUI???
I have removed the window manager XTC from the repos. The reason is because it isn't crucial right now to my end goals with this project. Once the kernel and userland is more stable, it will be added back.

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

### Third Party Software
The following third party software is included in this operating system by default. 

|Name|Author|URL|
|------|------|-----|
|newlib|Redhat|https://sourceware.org/newlib/|
|imple Scalable Font|bztsrc|https://gitlab.com/bztsrc/scalable-font|
