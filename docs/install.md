# Hardware Setup and Software Installation

In order to setup the DAQ system the following two steps are needed:

1. Install the YARR SW package
2. Install the custom PCIe kernel driver
3. Prepare and setup the DAQ hardware

NB for some hardware controllers there are extra dependencies requried,
for instance [NetIO](netio.md).

## Software installation

### Dependencies

- Make sure you have GCC version 7.0 or higher installed:
```bash
$ g++ --version
g++ (GCC) 7.2.1 20170829 (Red Hat 7.2.1-1)
```
- By default an older version of GCC is installed on CentOs7, you can install newer GCC versions via:
```bash
# 1. Install a package with repository for your system:
# On CentOS, install package centos-release-scl available in CentOS repository:
$ sudo yum install centos-release-scl

# On RHEL, enable RHSCL repository for you system:
$ sudo yum-config-manager --enable rhel-server-rhscl-7-rpms

# 2. Install the collection:
$ sudo yum install devtoolset-7
```
-- In order to use this newer version instead of your default one execute:
```bash
# Source the setup script
$ source /opt/rh/devtoolset-7/enable
# Add it to your bash_profile to enable it by default
$ echo "source /opt/rh/devtoolset-7/enable" >> ~/.bash_profile 
```
- If not installed before, you need some standard packages:
```bash
$ sudo yum install gnuplot texlive-epstopdf cmake3 zeromq zeromq-devel 
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

This repository uses the cmake build system in its usual manner.

#### Compile software with cmake
- Generate makefile
    - By default the minimal build is enabled, which builds only the Emulator and SPEC controller, if you want to run with additional controllers (e.g. NetIO, or Rogue) you have to enable them via a cmake flag (see below)
```bash
$ cd Yarr/
$ mkdir build
$ cd build
$ cmake3 ../
<Some text>
```
- In order to build with more controllers execute cmake with extra options
    - For all controllers: 
        - ``$ cmake3 -DYARR_CONTROLLERS_TO_BUILD=all ..``
    - For NetIO:
        - ``$ cmake3 -DYARR_CONTROLLERS_TO_BUILD=Spec;Emu;NetioHW``
    - For Rogue:
        - ``$ cmake3 -DYARR_CONTROLLERS_TO_BUILD=Spec;Emu;Rogue``
- Expert note: you can choose a specific toolchain via:
```bash
$ cmake3 ..  -DCMAKE_TOOLCHAIN_FILE=../cmake/linux-clang # requires clang installed on Linux
$ cmake3 ..  -DCMAKE_TOOLCHAIN_FILE=../cmake/linux-gcc # gcc 4.8 or higher
$ cmake3 ..  -DCMAKE_TOOLCHAIN_FILE=../cmake/rce-gcc # ARM/Archlinux on RCE
$ cmake3 ..  -DCMAKE_TOOLCHAIN_FILE=../cmake/macos-clang # MacOS build
```
- Compile the software
```bash
$ make -j4
<Lots of text>
$ make install
$ cd ..
```

## Next step

Install the PCIe kernel driver: [Kernel Driver Installation](kernel_driver.md)

