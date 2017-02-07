.. image:: http://readthedocs.org/projects/yarr/badge/?version=latest
    :target: http://yarr.readthedocs.io/en/latest/?badge=latest
    :alt: Documentation Status

======================
Yarr: Yet Another Rapid Readout
======================

Documentation
=====================
Documentation covering installation and usage can be found here http://yarr.rtfd.org (for devel branch http://yarr.readthedocs.org/en/devel/ )

Mailing list
=====================
Subscribe to the CERN mailing list: `yarr-user <https://e-groups.cern.ch/e-groups/EgroupsSubscription.do?egroupName=yarr-users>`_ 

Requirements
=====================
Hardware:
    - SPEC board: http://www.ohwr.org/projects/spec/wiki

Software:
    - SLC6 (Scientific Linux CERN 6) or CC7 (CERN CentOs 7)
    - GCC version 4.8 or higher
        - for example from devtoolset-4, instruction can be found here https://www.softwarecollections.org/en/scls/rhscl/devtoolset-4
    - QT5 for graphical user interface
    - Xilinx ISE 14.7 and `hdlmake <http://www.ohwr.org/projects/hdl-make/wiki>`_ (only to rebuild firmware)

Optional:
    - cmake version 2.8 or higher
        - for MacOS consider installing cmake from homebrew: http://brew.sh
    - Atlas RCE SDK for cross compilation and running on RCEs (HSIO2 or COB)
    - to install in /opt/AtlasRceSDK
        - sudo mkdir -p /opt/AtlasRceSDK
        - cd /opt/AtkasRceSDK/ ; curl -s  http://rceprojectportal.web.cern.ch/RceProjectPortal/software/SDK/V0.11.1.tar.gz | sudo tar xvfz - 
    - using CMake:
        - source /opt/AtkasRceSDK/V0.11.0/setup.sh # for RCEs
        - cd YARR/src
        - mkdir <builddir>
        - cd <buildir>
        - select one of the supported toolchains
            - make ..  -DCMAKE_TOOLCHAIN_FILE=../cmake/linux-clang # requires clang installed on Linux
            - make ..  -DCMAKE_TOOLCHAIN_FILE=../cmake/linux-gcc # gcc 4.8 or higher
            - make ..  -DCMAKE_TOOLCHAIN_FILE=../cmake/rce-gcc # ARM/Archlinux on RCE
            - make ..  -DCMAKE_TOOLCHAIN_FILE=../cmake/macos-clang # MacOS build
        - make