# Hardware Setup and Software Installation

In order to setup the DAQ system the following steps are needed:

1. Install the YARR SW package
2. Install the custom PCIe kernel driver
3. Prepare and setup the DAQ hardware

NB for some hardware controllers there are extra dependencies requried,
for instance [NetIO](netio.md).

## TLDR - Software installation 

Just want to install the latest version of the YARR software? Follow the quick install instructions here. In case of a new machine, or if you're not sure, follow the more detailed instructions below.

The main dependency is the compiler. Builds under clang are supported, but
with no particular version restriction. For gcc, we require at least gcc
version 9 and the build is currently tested against gcc 15 system. Anything
in between is expected to work.

<details> <summary> On rpm based systems you can install a particular gcc version...</summary>

```bash
# Source the setup script
$ source /opt/rh/devtoolset-9/enable
# Add it to your bash_profile to enable it by default
$ echo "source /opt/rh/devtoolset-9/enable" >> ~/.bash_profile
```
</details>

<details> <summary> From CVMFS you can install a particular gcc version...</summary>

```bash
source /cvmfs/sft.cern.ch/lcg/contrib/gcc/15/x86_64-el9/setup.sh
```
</details>


Clone the YARR repository to your local machine: 
```bash
$ git clone https://gitlab.cern.ch/Yarr/Yarr.git Yarr
```

Compile the minimal build, for more specific compilation options (e.g. NetIO), see below.
```bash
$ cd Yarr/
$ mkdir build
$ cd build
$ cmake3 ../
<Some text>
$ make -j4
<Lots of text>
$ make install
$ cd ..
```
In case of issues, please refer to the more detailed instructions below, or consult the [Troubleshooting](troubleshooting.md).

**Next step:**

- On a new machine, next install the PCIe kernel driver: [Kernel Driver Installation](kernel_driver.md)

- On a machine which already has the PCIe kernel driver installed, proceed to [Flashing the firmware](fw_guide.md)

## Software installation

### Dependencies for Centos 8

- If not installed before, you need some standard packages:

```bash
$ sudo yum install gnuplot texlive-epstopdf cmake3 elfutils-libelf-devel
```


### Dependencies for Alma 9
- If not installed before, you need some standard packages:
```bash
$ sudo yum install gnuplot texlive-epstopdf cmake
```


### Initialise repository
If you want to install the software in a new machine or want to make a new installation, then clone the repository to your local machine:
```bash
$ git clone https://gitlab.cern.ch/Yarr/Yarr.git Yarr
```
This will get the default master branch which contains the latest stable release. Use

```bash
$ cd Yarr
$ git tag
```
to list all available tagged versions. To use a specific tagged version from the list, do
```bash
$ cd Yarr
$ git checkout <tagged version>
```
The most up-to date development can be found in the devel branch.

- A version history and description can be found [here](version.md)
- Note, Yarr is also available in a [GitHub repository.](https://github.com/Yarr/ "GitHub repository"), but using the GitLab version is recommended as it is used for active development.


### Update the software version
If you already have an installed YARR version on your local machine, then just fetch the latest version to update it:

```bash
$ git fetch --all
$ git checkout <tagged version>
```
Clean all files from the `build` folder using
```bash
$ cd build
$ rm -rf *
```
then continue the installation using `cmake3` as detailled below.


### Compile the software

This repository uses the cmake build system in its usual manner.

#### Basic compilation

By default the minimal build is enabled, which builds only the Emulator and SPEC controller, if you want to run with additional controllers (e.g. NetIO) you have to enable them via a cmake flag (see below).

For the minimal build, simply execute the following: 

```bash
$ cd Yarr/
$ mkdir build
$ cd build
$ cmake3 ../
<Some text>
$ make -j4
<Lots of text>
$ make install
$ cd ..
```

**Next step:**
- On a new machine, next install the PCIe kernel driver: [Kernel Driver Installation](kernel_driver.md)
- On a machine which already has the PCIe kernel driver installed, proceed to [Flashing the firmware](fw_guide.md)

#### Compilation with additional options

- In order to build with more controllers execute cmake with extra options
    - For all controllers: 
        - ``$ cmake3 -DYARR_CONTROLLERS_TO_BUILD=all ..``
    - For NetIO:
        - ``$ cmake3 -DYARR_CONTROLLERS_TO_BUILD="Spec;Emu;NetioHW"``

- In order to specify specific hardware controller and/or front-end libraries to build,
one can provide an OR'ed chain of their names to the `SELECT_LIBS` CMake variable. For example, if the default list of hardware controllers is `YARR_CONTROLLERS_TO_BUILD="Spec;Emu;NetioHW"` and the default list of front-ends to build is `YARR_FRONT_ENDS_TO_BUILD="Fei4;Star;Rd53a;Rd53b"` one can specify that only the `Spec` hardware controller and `Rd53b` front-end libraries are built by doing:
```
    $ cmake3 -DSELECT_LIBS="Spec|Rd53b" ..
```
- In order to specify a subset of executables to be built, one can provide an OR'ed chain of the names of the executables to be built with the `SELECT_TOOLS` CMake variable. For example, to only build the `scanConsole` executable one can do:
```
    $ cmake3 -DSELECT_TOOLS=scanConsole ..
```
- Expert note: you can choose a specific toolchain via:
```bash
$ cmake3 ..  -DCMAKE_TOOLCHAIN_FILE=../cmake/linux-clang # requires clang installed on Linux
$ cmake3 ..  -DCMAKE_TOOLCHAIN_FILE=../cmake/linux-gcc
$ cmake3 ..  -DCMAKE_TOOLCHAIN_FILE=../cmake/macos-clang # MacOS build
```
- As before, finally compile the software: 
```bash
$ make -j4
<Lots of text>
$ make install
$ cd ..
```

Some of the options and dependencies are described in [the instructions how to use YARR as a dependency](consuming_yarr.md). Overview of most option switches:

| option | default | comment | depends on option  |
|------------|------|---------------|----------------------------|
| BUILD_TESTS:BOOL | OFF | Build the YARR test suite | |
| CMAKE_BUILD_TYPE:STRING | "" | Standard CMake setting: used by Yarr as always a release with stripped debug infos is built| |
| CMAKE_CXX_STANDARD:STRING | 17 | Standard CMake setting: C++ standard to use | |
| CMAKE_INSTALL_PREFIX:PATH | in source tree | Standard CMake setting: install path prefix, standard in source installation tries not overwrite anything, better to do an out-of-tree installation | |
| LIBFABRIC_CONFIGURE_OPTS:STRING | "" | Extra configure options for in-built libfabric (just in case) | NetioHW enabled and builtin libfabric needed or forced |
| NETIO4_BUILD_TESTS:BOOL | OFF | Build netio4 executables | NetioHW enabled |
| NETIO4_FORCE_USE_BUILTIN_LIBFABRIC:BOOL | ON | Force built-in libfabric instead of system provided | NetioHW enabled |
| NETIO4_FORCE_USE_BUILTIN_ZEROMQ:BOOL | ON | Force built-in ZeroMQ instead of system provided | NetioHW enabled and netio4 tests enabled | 
| YARR_ACTIVE_LOGGER_LEVEL:STRING | DEBUG | SPDLOG_ACTIVE_LEVEL below which logger macros are disabled at build time. One of TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL, OFF. | |
| YARR_CONTROLLERS_TO_BUILD:STRING | all | Semicolon-separated list of controllers to build, or "all". | |
| YARR_DEBUG_PRINT_TARGETS:BOOL | ON | Prints all targets and their properties. Useful for debugging and for writing a dependency on Yarr. | |
| YARR_EMULATORS_TO_BUILD:STRING | "StarEmu;Fei4Emu;Rd53aEmu;Itkpixv2Emu" | Front-end specific emulators to build (if Emu in controller list). | Emu in YARR_CONTROLLERS_TO_BUILD |
| YARR_ENABLE_PYTHON:BOOL | ON | Build python bindings | |
| YARR_FORCE_FETCHCONTENT_SPDLOG:BOOL | OFF | Force built-in spdlog | |
| YARR_FORCE_OWN_INSTALL_PREFIX:BOOL | OFF | Force Yarr to use own install prefix even as subdirectory | |
| YARR_FRONT_ENDS_TO_BUILD:STRING | "Fei4;Rd53a;Star;Rd53b;Itkpixv2" | Semicolon-separated list of controllers to build, or "all" | |
| YARR_USE_FETCHCONTENT_SPDLOG:BOOL | ON | Use FetchContent to get spdlog if not found. | |
| Felix_ROOT | "" | path to pre-compiled FELIX, like /felix-05-02-00-rm5-stand-alone/x86_64-el9-gcc15-opt (default "" will build from git) | |
