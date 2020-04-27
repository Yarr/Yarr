![docs](http://readthedocs.org/projects/yarr/badge/?version=latest)

![https://gitlab.cern.ch/YARR/YARR/pipelines](https://gitlab.cern.ch/YARR/YARR/badges/master/pipeline.svg)

![logo](docs/images/logo_blue_inv.png)

# YARR: Yet Another Rapid Readout

## Documentation

For details please refer to the documentation covering installation and usage, which can be found here http://cern.ch/yarr

This README only includes quick install guide.

(If you are working with the devel branch refer to http://cern.ch/yarr/devel/ )

## Mailing list

Users should subscribe to the CERN mailing list to receive announcements for important updates: [yarr-user](https://e-groups.cern.ch/e-groups/EgroupsSubscription.do?egroupName=yarr-users)

Developers and potential developers please refer to [Contribution](CONTRIBUTING.md) guide.

## Requirements

### Software:

- CentOS 7
- cmake 3.6
- GCC version 7 or higher
    - for example from devtoolset-7, instruction can be found here https://www.softwarecollections.org/en/scls/rhscl/devtoolset-7
- Some misc packages (can be installed via yum):
    - gnuplot
    - texlive-epstopdf
    - zeromq, zeromq-devel (for rogue and netio controller)

## Quick minimal Install Guide:

- Builds spec and emu controller for mininmal dependencies
- Clone from git 
	- ``$ git clone https://gitlab.cern.ch/YARR/YARR.git Yarr``
- Compilation:
    - ``$ cd Yarr``
    - ``$ mkdir build``
    - ``$ cd build``
    - ``$ source scl_source enable devtoolset-7``
    - ``$ cmake3 ..``
    - ``$ make install -j4``
    - ``$ cd ..``
- Running
    - execute programs from the repository top folder

### Building additional controllers

- In order to build with more controllers execute cmake with extra options
    - For all controllers: 
        - ``$ cmake3 -DYARR_CONTROLLERS_TO_BUILD=all ..``
    - For NetIO:
        - ``$ cmake3 -DYARR_CONTROLLERS_TO_BUILD="Spec;Emu;NetioHW"``
    - For Rogue:
        - ``$ cmake3 -DYARR_CONTROLLERS_TO_BUILD="Spec;Emu;Rogue"``

### RCE Guide
- for ARM target cross compilers are provided by the RCE_SDK
    - installtion instructions: https://twiki.cern.ch/twiki/bin/viewauth/Atlas/RCEGen3SDK
- using CMake:
    - ``$ source /opt/rce/setup.sh (for RCE cross compilation setup)``
    - ``$ export CENTOS7_ARM32_ROOT=/opt/rce/rootfs/centos7 #(points to cross installed CentOS7)``
    - ``$ export CENTOS7_ARM64_ROOT=/opt/rce/rootfs/centos7_64 #for ZCU102 ``
    - ``$ cd build``
    - select one of the supported toolchains
        - ``$ cmake3 .. -DYARR_CONTROLLERS_TO_BUILD=all -DCMAKE_TOOLCHAIN_FILE=../cmake/linux-clang # requires clang installed on Linux ``
        - ``$ cmake3 .. -DYARR_CONTROLLERS_TO_BUILD=all -DCMAKE_TOOLCHAIN_FILE=../cmake/linux-gcc # gcc 4.8 or higher ``
        - ``$ cmake3 .. -DYARR_CONTROLLERS_TO_BUILD=all -DCMAKE_TOOLCHAIN_FILE=../cmake/rce-arm32 # ARM32/Centos7 on RCE ``
        - ``$ cmake3 .. -DYARR_CONTROLLERS_TO_BUILD=all -DCMAKE_TOOLCHAIN_FILE=../cmake/rce-arm64 # ARM64/Centos7 on zcu102 ``
    - ``$ make -j8 install ``

### Running tests

While developing, it might be useful to run some unit tests. These are run
by default in the CI on gitlab, but can also be run locally:

- cd build
- make test

This runs the test_main binary, which gathers the tests found in src/tests.
