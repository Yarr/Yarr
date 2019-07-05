![docs](http://readthedocs.org/projects/yarr/badge/?version=latest)
![https://gitlab.cern.ch/YARR/YARR/pipelines](https://gitlab.cern.ch/YARR/YARR/badges/master/pipeline.svg)

![logo](docs/images/logo_blue_inv.png)

# YARR: Yet Another Rapid Readout

## Documentation

Documentation covering installation and usage can be found here http://yarr.rtfd.org (for devel branch http://yarr.readthedocs.org/en/devel/ )

## Mailing list

Users should subscribe to the CERN mailing list to receive announcements for important updates: [yarr-user](https://e-groups.cern.ch/e-groups/EgroupsSubscription.do?egroupName=yarr-users)

Developers and potential developers please refer to [Contribution](CONTRIBUTING.md) guide.

## Requirements

### Hardware:

- Please refer to the [documentation](http://readthedocs.org/projects/yarr/badge/?version=latest) for details

### Software:

- CentOS 7
- cmake 3.6
- GCC version 7 or higher
    - for example from devtoolset-7, instruction can be found here https://www.softwarecollections.org/en/scls/rhscl/devtoolset-7
- Some misc packages:
    - gnuplot
    - texlive-epstopdf
    - zeromq, zeromq-devel

## Quick Install Guide:
- Clone from git 
	- git clone https://gitlab.cern.ch/YARR/YARR.git
- Compilation:
    - ``$ mkdir build``
    - ``$ cd build``
    - ``$ cmake3 ..``
    - ``$ make install -j4``
    - ``$ cd ..``
- Running
    - execute programs from the repository top folder

### RCE Guide
    - for ARM target cross compilers are provided by the RCE_SDK
        - installtion instructions: https://twiki.cern.ch/twiki/bin/viewauth/Atlas/RCEGen3SDK
    - using CMake:
        - source /opt/rce/setup.sh (for RCE cross compilation setup)
        - export CENTOS7_ARM32_ROOT=/opt/rce/rootfs/centos7 #(points to cross installed CentOS7)
        - export CENTOS7_ARM64_ROOT=/opt/rce/rootfs/centos7_64 #for ZCU102 
        - cd build
        - select one of the supported toolchains
            - cmake3 ..  -DCMAKE_TOOLCHAIN_FILE=../cmake/linux-clang # requires clang installed on Linux
            - cmake3 ..  -DCMAKE_TOOLCHAIN_FILE=../cmake/linux-gcc # gcc 4.8 or higher
            - cmake3 ..  -DCMAKE_TOOLCHAIN_FILE=../cmake/rce-arm32 # ARM32/Centos7 on RCE
            - cmake3 ..  -DCMAKE_TOOLCHAIN_FILE=../cmake/rce-arm64 # ARM64/Centos7 on zcu102
        - make -j4 install
