# Hardware Setup and Software Installation

In order to setuo the DAQ system the following two steps are needed:

1. Install the YARR SW package
2. Install the custom PCIe kernel driver
3. Prepare and setup the DAQ hardware

## Software installation

### Dependencies

- Make sure you have GCC version 4.8 or higher installed:
```bash
$ g++ --version
g++ (GCC) 4.8.3 20140911 (Red Hat 4.8.3-9)
Copyright (C) 2013 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```
- If not a specifc version can also be sourced from AFS (CERN AFS access necessary):
```bash
$ source /afs/cern.ch/sw/lcg/contrib/gcc/4.8/x86_64-slc6/setup.sh
```
- Newer GCC version are supported as well and can be installed via
- If not installed before, you need some standard packages:
```bash
$ sudo yum install gnuplot texlive-epstopdf cmake
```
### Initialise repository

- Clone the repository to your local machine:
```bash
$ git clone https://github.com/Yarr/Yarr.git Yarr
```
or
```bash
$ git clone https://gitlab.cern.ch/Yarr/Yarr.git Yarr
```
- The master branch should contain the latest stable release, the most up-to date version can be found in the devel branch
- A version history and description can be found [here](version.md)

### Compile the software

There are two ways to compile the software, either via a simple Makefile or via cmake. Pick your poison.
If in doubt, use the simple Makefile method.

#### Compile software with Makefile
- Compile the software:
```bash
$ cd Yarr/src
$ make -j4
<Lots of text>
```
#### Compile software with cmake
- Generate makefile
```bash
$ cd Yarr/src
$ mkdir build
$ cd build
$ cmake ../
<Some text>
```
- Expert note: you can choose a specific toolchain via:
```bash
$ cmake ..  -DCMAKE_TOOLCHAIN_FILE=../cmake/linux-clang # requires clang installed on Linux
$ cmake ..  -DCMAKE_TOOLCHAIN_FILE=../cmake/linux-gcc # gcc 4.8 or higher
$ cmake ..  -DCMAKE_TOOLCHAIN_FILE=../cmake/rce-gcc # ARM/Archlinux on RCE
$ cmake ..  -DCMAKE_TOOLCHAIN_FILE=../cmake/macos-clang # MacOS build
```
- Compile the software
```bash
$ make -j4
<Lots of text>
```

## Next step

Install the PCIe kernel driver: [Kernel Driver Installation](kernel_driver.md)

