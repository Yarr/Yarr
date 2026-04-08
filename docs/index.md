# YARR: Yet another Rapid Readout

## What is YARR?
YARR is a readout system based around the concept of moving intelligence from the FPGA firmware into the host computer software. The role of the FPGA is the one of a reconfigurable I/O interface and not anymore the one of a hardware accelerator. YARR supports multiple types of FPGA platforms:

* Simple PCIe Carrier Cards (SPEC): XpressK7 160/325, Trenz TEF1001 R1/R2, Numato Nereid K7 and Xilinx KC705
* FELIX-STAR + NETIO-NEXT
* BDAQ Hardware

The currently supported readout chips are:

* FE-I4B
* FE65-P2
* RD53A
* ITkPixV1 (RD53B)
* ITkPixV2 (RD53C)
* ABC/HCC STAR

## Support

Support for YARR can be found in the [YARR Matter Most channel.](https://mattermost.web.cern.ch/yarr/ "YARR MatterMost")


## Links to docs in development

* Future docs: https://yarr.web.cern.ch/yarr/devel/
* Developer docs: https://yarr.web.cern.ch/doxygen/devel/
* Coverage: https://yarr.web.cern.ch/yarr/devel/coverage/

## Folder Structure
```bash
├── configs : templates and examples for various config files
│   ├── connectivity
│   ├── controller
│   ├── defaults
│   ├── emulator
│   ├── logging
│   ├── scans : scan config for various front-end types
├── docs : this documentation
├── python : python scripts using bindings
├── scripts : various other scripts for QoL
├── src : main source code
│   ├── libBdaq : BDAQ platform controller
│   ├── libEmu : Front-End Emulator library
│   ├── libFei4 : FEI4 front-end library
│   ├── libFei4Emu : FEI4 emulator
│   ├── libFelixClient : FELIX client thread platform controller
│   ├── libItkpixv2 : ITkPixV2 front-end library
│   ├── libItkpixv2Emu : ITkPixV2 emulator
│   ├── libItsdaqFW : ITSDAQ platform controller
│   ├── libRd53a : RD53A fron-end library
│   ├── libRd53aEmu : RD53A emulator
│   ├── libRd53b : RD53B front-end library
│   ├── libSpec : SPEC platform controller
│   ├── libStar : STAR chips (HCC, ABC) front-end library
│   ├── libStarEmu : STAR chips emulator
│   ├── libUtil : utilities
│   ├── libYarr : Primary Yarr library
│   ├── python : python bindings
│   ├── tools : Primary Yarr executeables
```


