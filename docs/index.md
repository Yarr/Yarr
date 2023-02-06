# YARR: Yet another Rapid Readout

## What is YARR?
YARR is a readout system based around the concept of moving intelligence from the FPGA firmware into the host computer software. The role of the FPGA is the one of a reconfigurable I/O interface and not anymore the one of a hardware accelerator. YARR supports multiple types of FPGA platforms:

* COTS PCIe FPGA cards: CERN SPEC, XpressK7, Trenz TEF1001, and Xilinx KC705
* SLAC's HSIO2/RCE
* FELIX + NETIO
* FELIX-STAR + NETIO-NEXT
* ATLAS IBL BOC
* Wuppertal's KU040
* BDAQ Hardware 

The currently supported readout chips are:

* FE-I4B
* FE65-P2
* RD53A
* RD53B (ITkPixV1)
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
    |-- libBoc : ATLAS IBL BOC hw driver
    |-- libEmu : FE Emulator hw driver
    |-- libFe65p2 : FE65p2 implementation
    |-- libFei4 : FE-I4B implementation
    |-- libKU040 : KU040 hw driver
    |-- libNetioHW : FELIX driver
    |-- libFelixClient: FELIX driver for felix-star and NetIO-next
    |-- libRce : HSIO2 hw driver
    |-- libRd53a: RD53a implementation
    |-- libRogue: Rogue HW controller
    |-- libSpec : PCIe hw driver
    |-- libUtil : Suppert library
    |-- libYarr : YARR core libraries 
    |-- tools : Main executeables
    `-- util : Utility scrips and files
|-- plotting : Scripts to interface with extenral libraries (e.g. ROOT), primarily to produce plots
`-- scripts : bash scripts for setup or automation
```


