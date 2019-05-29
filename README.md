![docs](http://readthedocs.org/projects/yarr/badge/?version=latest)

# YARR: Yet Another Rapid Readout

## Documentation

Documentation covering installation and usage can be found here http://yarr.rtfd.org (for devel branch http://yarr.readthedocs.org/en/devel/ )

## Mailing list

Subscribe to the CERN mailing list: [yarr-user](https://e-groups.cern.ch/e-groups/EgroupsSubscription.do?egroupName=yarr-users)

## Supported Hardware:
The YARR SW supports multiple hardware platforms which have been integrated:

- YARR PCIe cards (SPEC and XpressK7): link
- IBL BOC: link
- Wup KU40: link
- Rogue HSIO2/ZCU102: link

## Requirements
Hardware:

- Update me

Software:

- CentOS 7
- GCC version 7 or higher
    - for example from devtoolset-7, instruction can be found here https://www.softwarecollections.org/en/scls/rhscl/devtoolset-7

Quick Install Guide:
- download from git 
	- git clone -b devel_rogue ssh://git@gitlab.cern.ch:7999/YARR/YARR.git
- Using cmake:
    - cmake version 3 or higher
    - for ARM target cross compilers are provided by the RCE_SDK
        - installtion instructions: https://twiki.cern.ch/twiki/bin/viewauth/Atlas/RCEGen3SDK
    - using CMake:
        - source /opt/rce/setup.sh (for RCE cross compilation setup)
        - export CENTOS7_ARM32_ROOT=/opt/rce/rootfs/centos7 #(points to cross installed CentOS7)
        - export CENTOS7_ARM64_ROOT=/opt/rce/rootfs/centos7_64 #for ZCU102 
        - cd build
        - select one of the supported toolchains
            - cmake3 ..  -DCMAKE_TOOLCHAIN_FILE=../src/cmake/linux-clang # requires clang installed on Linux
            - cmake3 ..  -DCMAKE_TOOLCHAIN_FILE=../src/cmake/linux-gcc # gcc 4.8 or higher
            - cmake3 ..  -DCMAKE_TOOLCHAIN_FILE=../src/cmake/rce-arm32 # ARM32/Centos7 on RCE
            - cmake3 ..  -DCMAKE_TOOLCHAIN_FILE=../src/cmake/rce-arm64 # ARM64/Centos7 on zcu102
            - cmake3 ..  -DCMAKE_TOOLCHAIN_FILE=../src/cmake/macos-clang # MacOS build
        - make
