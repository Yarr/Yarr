![docs](http://readthedocs.org/projects/yarr/badge/?version=latest)

# YARR: Yet Another Rapid Readout

## Documentation

Documentation covering installation and usage can be found here http://yarr.rtfd.org (for devel branch http://yarr.readthedocs.org/en/devel/ )

## Mailing list

Users should subscribe to the CERN mailing list to receive announcements for important updates: [yarr-user](https://e-groups.cern.ch/e-groups/EgroupsSubscription.do?egroupName=yarr-users)

Developers and potential developers please refer to [Contribution](CONTRIBUTING.md) guide.

## Quick Install Guide

### Hardware:

- Please refer to the [documentation](http://readthedocs.org/projects/yarr/badge/?version=latest) for details

### Software:

Requirements:

- CC7 (CERN CentOs 7) 
- GCC version 7.0 or higher
    - for example from devtoolset-7, instruction can be found [here](https://yarr.readthedocs.io/en/latest/install/#hardware-setup-and-software-installation)

Quick Default Install Guide:

- Using make:
    - ``cd YARR/src``
    - ``make``
- Using cmake:
    - ``cd YARR``
    - ``mkdir build``
    - ``cd build``
    - or select one of the supported toolchains
        - make ..  -DCMAKE_TOOLCHAIN_FILE=../cmake/linux-clang # requires clang installed on Linux
        - make ..  -DCMAKE_TOOLCHAIN_FILE=../cmake/linux-gcc # gcc 4.8 or higher
        - make ..  -DCMAKE_TOOLCHAIN_FILE=../cmake/rce-gcc # ARM/Archlinux on RCE
        - make ..  -DCMAKE_TOOLCHAIN_FILE=../cmake/macos-clang # MacOS build
    - ``make``


Quick RCE Install Guide:

- source /opt/AtkasRceSDK/V0.11.0/setup.sh # for RCEs
- Atlas RCE SDK for cross compilation and running on RCEs (HSIO2 or COB)
- to install in /opt/AtlasRceSDK
    - sudo mkdir -p /opt/AtlasRceSDK
    - cd /opt/AtkasRceSDK/ ; curl -s  http://rceprojectportal.web.cern.ch/RceProjectPortal/software/SDK/V0.11.1.tar.gz | sudo tar xvfz - 
        

Quick NetIO Install Guide:
