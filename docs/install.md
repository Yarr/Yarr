# Hardware Setup and Software Installation

In order to setup the DAQ system the following two steps are needed:

1. Install the YARR SW package
2. Install the custom PCIe kernel driver
3. Prepare and setup the DAQ hardware

NB for some hardware controllers there are extra dependencies requried,
for instance [NetIO](netio.md).

## TLDR - Software installation 

Just want to install the latest version of the YARR software? Follow the quick install instructions here. In case of a new machine, or if you're not sure, follow the more detailed instructions below. 

Enable GCC version 9.0 or higher: 
```bash
# Source the setup script
$ source /opt/rh/devtoolset-9/enable
# Add it to your bash_profile to enable it by default
$ echo "source /opt/rh/devtoolset-9/enable" >> ~/.bash_profile
```

Clone the YARR repository to your local machine: 
```bash
$ git clone https://gitlab.cern.ch/Yarr/Yarr.git Yarr
```

Compile the minimal build, for more specific compilation options (e.g. NetIO, or Rogue), see below. 
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

- On a machine which already has the PCIe kernel driver installed, proceed to [Flashing the firmware](pcie.md)

## Software installation

### Dependencies for Centos 7

- Make sure you have GCC version 9.0 or higher installed:

```bash
$ g++ --version
g++ (GCC) 9.3.1 20200408 (Red Hat 9.3.1-2)
```

- By default an older version of GCC is installed on CentOs7, you can install newer GCC versions via:

```bash
# 1. Install a package with repository for your system:

# On CentOS, install package centos-release-scl available in CentOS repository:
$ sudo yum install centos-release-scl

# On RHEL, enable RHSCL repository for you system:
$ sudo yum-config-manager --enable rhel-server-rhscl-9-rpms

# 2. Install the collection:
$ sudo yum install devtoolset-9
```

- In order to use this newer version instead of your default one execute:

```bash
# Source the setup script
$ source /opt/rh/devtoolset-9/enable
# Add it to your bash_profile to enable it by default
$ echo "source /opt/rh/devtoolset-9/enable" >> ~/.bash_profile
```

- If not installed before, you need some standard packages:

```bash
$ sudo yum install gnuplot texlive-epstopdf cmake3 zeromq zeromq-devel 
```

### Dependencies for Centos 8

- If not installed before, you need some standard packages:

```bash
$ sudo yum install gnuplot texlive-epstopdf cmake3 elfutils-libelf-devel
```

### Initialise repository
- Clone the repository to your local machine:
```bash
$ git clone https://gitlab.cern.ch/Yarr/Yarr.git Yarr
```
- The master branch should contain the latest stable release, the most up-to date version can be found in the devel branch
- A version history and description can be found [here](version.md)
- Note, Yarr is also available in a [GitHub repository.](https://github.com/Yarr/ "GitHub repository"), but using the GitLab version is recommended and what is used for active development. 

### Compile the software

This repository uses the cmake build system in its usual manner.

#### Basic compilation

By default the minimal build is enabled, which builds only the Emulator and SPEC controller, if you want to run with additional controllers (e.g. NetIO, or Rogue) you have to enable them via a cmake flag (see below). 

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
- On a machine which already has the PCIe kernel driver installed, proceed to [Flashing the firmware](pcie.md)

#### Compilation with additional options

- In order to build with more controllers execute cmake with extra options
    - For all controllers: 
        - ``$ cmake3 -DYARR_CONTROLLERS_TO_BUILD=all ..``
    - For NetIO:
        - ``$ cmake3 -DYARR_CONTROLLERS_TO_BUILD=Spec;Emu;NetioHW``
    - For Rogue:
        - ``$ cmake3 -DYARR_CONTROLLERS_TO_BUILD=Spec;Emu;Rogue``

- In order to specify specific hardware controller and/or front-end libraries to build,
one can provide an OR'ed chain of their names to the `SELECT_LIBS` CMake variable. For example, if the default list of hardware controllers is `YARR_CONTROLLERS_TO_BUILD=Spec;Emu;NetioHW` and the default list of front-ends to build is `YARR_FRONT_ENDS_TO_BUILD=Fei4;Star;Rd53a;Rd53b` one can specify that only the `Spec` hardware controller and `Rd53b` front-end libraries are built by doing:
```
    $ cmake3 -DSELECT_LIBS=Spec|Rd53b ..
```
- In order to specify a subset of executables to be built, one can provide an OR'ed chain of the names of the executables to be built with the `SELECT_TOOLS` CMake variable. For example, to only build the `scanConsole` executable one can do:
```
    $ cmake3 -DSELECT_TOOLS=scanConsole ..
```
- In order to skip the installation of the plotting tools, switch on the option `DISABLE_PLOTTING_TOOLS` (default is `OFF`):

```
    $ cmake3 -DDISABLE_PLOTTING_TOOLS=ON ..
```
- Expert note: you can choose a specific toolchain via:
```bash
$ cmake3 ..  -DCMAKE_TOOLCHAIN_FILE=../cmake/linux-clang # requires clang installed on Linux
$ cmake3 ..  -DCMAKE_TOOLCHAIN_FILE=../cmake/linux-gcc # gcc 4.8 or higher
$ cmake3 ..  -DCMAKE_TOOLCHAIN_FILE=../cmake/rce-gcc # ARM/Archlinux on RCE
$ cmake3 ..  -DCMAKE_TOOLCHAIN_FILE=../cmake/macos-clang # MacOS build
```
- As before, finally compile the software: 
```bash
$ make -j4
<Lots of text>
$ make install
$ cd ..
```


