# YARR: Yet another Rapid Readout

## What is YARR?
YARR is a readout system based around the concept of moving intelligence from the FPGA firmware into the host computer software. The role of the FPGA is the one of a reconfigurable I/O interface and not anymore the one of a hardware accelerator. YARR supports multiple types of FPGA platforms:

* Simple PCIe Carrier Cards (SPEC): XpressK7 160/325, Trenz TEF1001 R1/R2, Numato Nereid K7 and Xilinx KC705
* FELIX + NETIO
* FELIX-STAR + NETIO-NEXT
* BDAQ Hardware 

The currently supported readout chips are:

* FE-I4B
* FE65-P2
* RD53A
* ITkPixV1 (RD53B)
* ITkPixV2 (RD53C)
* ABC/HCC STAR

There is very preliminary support for Star chips (strips).

## Support

Support for YARR can be found in the [YARR Matter Most channel.](https://mattermost.web.cern.ch/yarr/ "YARR MatterMost")


## Links to docs in development

* Future docs: https://yarr.web.cern.ch/yarr/devel/
* Developer docs: https://yarr.web.cern.ch/doxygen/devel/
* Coverage: https://yarr.web.cern.ch/yarr/devel/coverage/

## Folder Structure
```bash
.
|-- configs : Config templates
|-- doc : Documentation
|-- src : Main software dir
    |-- kernel : Custom PCIe kernel driver
    |-- libBdaq : BDAQ hw driver
    |-- libEmu : FE Emulator hw driver
    |-- libFei4 : FE-I4B implementation
    |-- libStar : Strips Star implementation
    |-- libNetioHW : FELIX driver
    |-- libFelixClient: FELIX driver for felix-star and NetIO-next
    |-- libRd53a: RD53a implementation
    |-- libRd53b: RD53B/ItkPixV1 implementation
    |-- libItkpixv2: RD53C/ItkPixV2 implementation
    |-- libSpec : PCIe hw driver
    |-- libUtil : Suppert library
    |-- libYarr : YARR core libraries 
    |-- tools : Main executeables
    `-- util : Utility scrips and files
|-- plotting : Scripts to interface with extenral libraries (e.g. ROOT), primarily to produce plots
`-- scripts : bash scripts for setup or automation
```


