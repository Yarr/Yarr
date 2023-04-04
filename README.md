![logo](docs/images/logo_blue_inv.png)

# YARR: Yet Another Rapid Readout

## Documentation

For details please refer to the documentation covering installation and usage, which can be found here http://cern.ch/yarr

This README only includes quick install guide.

(If you are working with the devel branch refer to http://cern.ch/yarr/devel/ and see the current coverage report at https://yarr.web.cern.ch/yarr/devel/coverage/)

## Mailing list

Users should subscribe to the CERN mailing list to receive announcements for important updates: [yarr-user](https://e-groups.cern.ch/e-groups/EgroupsSubscription.do?egroupName=yarr-users)

Developers and potential developers please refer to [Contribution](CONTRIBUTING.md) guide.

## Requirements

### Software:

- CentOS 7/8 or Ubuntu 20.04 LTS
- cmake 3.14 or higher
- GCC version 9
    - for example from devtoolset-9
- Some misc packages (can be installed via yum):
    - gnuplot
    - texlive-epstopdf
    - zeromq, zeromq-devel (for rogue and netio controller)
    - boost-devel for BDAQ
    - ROOT for plotting tools

## Quick minimal Install Guide:

- Builds spec and emu controller for mininmal dependencies on Centos 7
- Build recipes for other OS can be found in docker/<OS>/Dockerfile
- Clone from git
	- ``$ git clone https://gitlab.cern.ch/YARR/YARR.git Yarr``
- Compilation:
    - ``$ source scl_source enable devtoolset-9``
    - ``cd Yarr``
    - ``$ cmake3 -S ./ -B build`` or ``$ cmake3 -S ./ -B build -DYARR_CONTROLLERS_TO_BUILD=all``
    - ``$ cmake3 --build build -j4``
    - ``$ cmake3 --install build -j4``
- Running
    - execute programs from the repository top folder

### Building additional controllers

- In order to build with more controllers execute cmake with extra options
    - For all controllers:
        - ``-DYARR_CONTROLLERS_TO_BUILD=all``
    - For NetIO:
        - ``-DYARR_CONTROLLERS_TO_BUILD="Spec;Emu;NetioHW"``
    - For Rogue:
        - ``-DYARR_CONTROLLERS_TO_BUILD="Spec;Emu;Rogue"``

While developing, it might be useful to run some unit tests. These are run
by default in the CI on gitlab, but can also be run locally:

- ``cd build``
- ``make test``

This runs the test_main binary, which gathers the tests found in src/tests.
