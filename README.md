![logo](docs/images/logo_blue_inv.png)

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.15007378.svg)](https://doi.org/10.5281/zenodo.15007378)
[![pipeline status](https://gitlab.cern.ch/YARR/YARR/badges/master/pipeline.svg)](https://gitlab.cern.ch/YARR/YARR/-/pipelines)

# YARR: Yet Another Rapid Readout

YARR is a C++ data acquisition (DAQ) framework for ATLAS pixel and strip detector ASICs. It provides a unified interface to configure chips, run calibration scans, and collect data via multiple hardware backends — all driven by JSON configuration files. It is used by detector groups working with the ATLAS Inner Tracker (ITk) and related testbeam/lab setups.

### Supported hardware

**Front-end chips** (any chip works with any controller):
- RD53B / ITkPixV2
- RD53A
- FEI4
- Star (strip detector endpoint comprising ABC and HCC chips)

**Hardware controllers:**
- Spec PCIe card
- FELIX
- BDAQ
- Software emulators (no hardware needed)

## Documentation

For details please refer to the [documentation](http://cern.ch/yarr) covering installation and usage.

This README only includes a quick install guide.

(If you are working with the devel branch refer to the [devel docs](http://cern.ch/yarr/devel/) and see the current [coverage report](https://yarr.web.cern.ch/yarr/devel/coverage/).)

## Mailing list

Users should subscribe to the CERN mailing list to receive announcements for important updates: [yarr-users](https://e-groups.cern.ch/e-groups/EgroupsSubscription.do?egroupName=yarr-users)

Developers and potential developers please refer to [Contribution](CONTRIBUTING.md) guide.

## Requirements

### Software:

- Alma 9 (primary supported OS; see `docker/` for build recipes on other distributions)
- cmake 3.14 or higher
- GCC version 11 or higher, C++20
- Some misc packages (can be installed via yum):
    - gnuplot, texlive-epstopdf (for built-in plotting)
    - boost-devel (for BDAQ)

## Quick minimal Install Guide:

- Builds spec and emu controllers with minimal dependencies on Alma 9
- Build recipes for other OS can be found in docker/<OS>/Dockerfile
- Clone from git
	- ``$ git clone https://gitlab.cern.ch/YARR/YARR.git Yarr``
- Compilation (default front-end and controller classes):
    - ``$ cd Yarr``
    - ``$ mkdir build && cd build``
    - ``$ cmake ..``
    - ``$ make -j$(nproc) install``
    - ``$ cd ..``
- Running
    - execute programs from the repository top folder like:
    - ``$ bin/scanConsole <...>``

### Quick start: run a scan with the emulator

No hardware required — the software emulator lets you verify the build end-to-end:

```bash
bin/scanConsole \
  -r configs/controller/emuCfg_itkpixv2.json \
  -c configs/connectivity/example_itkpixv2_setup.json \
  -s configs/scans/itkpixv2/std_digitalscan.json
```

Scan output is written to `data/<runnumber>_std_digitalscan/`.

### Building additional controllers

- In order to build with more controllers execute cmake with extra options
    - For all front-ends & controllers:
        - ``$ cmake -DYARR_FRONT_ENDS_TO_BUILD=all -DYARR_CONTROLLERS_TO_BUILD=all ..``
    - To add FELIX support:
        - ``$ cmake -DYARR_CONTROLLERS_TO_BUILD="Spec;Emu;FelixClient" ..``
    - List of all possible front-ends:
        - ``Fei4;Rd53a;Star;Rd53b;Itkpixv2``
    - List of all possible controllers:
        - ``Spec;Emu;StarEmu;Fei4Emu;Rd53aEmu;Itkpixv2Emu;Bdaq;FelixClient;ItsdaqFW``

While developing, it might be useful to run some unit tests. These are run
by default in the CI on gitlab, but can also be run locally:

- ``$ cd build``
- ``$ cmake -DBUILD_TESTS=on ..``
- ``$ make -j$(nproc) install``
- ``$ cd ..``
- ``$ bin/testYarr && bin/testUtil``

## License

YARR is distributed under the [GNU General Public License v2.0](LICENSE.txt).

## Citation

If you use YARR in your research, please cite it using the DOI above:

> T. Heim et al., *YARR — Yet Another Rapid Readout*, [https://doi.org/10.5281/zenodo.15007378](https://doi.org/10.5281/zenodo.15007378)
