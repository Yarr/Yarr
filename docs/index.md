# YARR: Yet another Rapid Readout

## What is YARR?
YARR is a readout system based around the concept of moving intelligence from the FPGA firmware into the host computer software. The role of the FPGA is the one of a reconfigurable I/O interface and not anymore the one of a hardware accelerator. YARR supports multiple types of FPGA platforms:

* COTS PCIe FPGA cards: CERN SPEC, XpressK7, Trenz TEF1001, and Xilinx KC705
* SLAC's HSIO2/RCE
* FELIX + NETIO
* ATLAS IBL BOC
* Wuppertal's KU040

The currently supported readout chips are:

* FE-I4B
* FE65-P2
* RD53A
* ABC/HCC STAR

There is very preliminary support for Star chips (strips).

## Support

Support for YARR can be found in the [YARR Matter Most channel.](https://mattermost.web.cern.ch/yarr/ "YARR MatterMost")


## Content
   
* [Software Installation](install.md)
* PCIe Installation
    * [PCIe Kernel Driver Installation](kernel_driver.md)
    * [PCIe Firmware Setup](pcie.md)
    * [External PCIe](pcie_ext.md)
* [ScanConsole](scanconsole.md)
    * [FE-I4](fei4.md)
    * [FE65-P2](fe65p2.md)
    * [RD53A](rd53a.md)
    * [Star Strips](star.md)
* [Troubleshooting](troubleshooting.md)


## Folder Structure
```bash
.
|-- configs : Config templates
|-- doc : Documentation
|-- src : Main software dir
    |-- kernel : Custom PCIe kernel driver
    |-- libBoc : ATLAS IBL BOC hw driver
    |-- libEmu : FE Emulator hw driver
    |-- libFe65p2 : FE65p2 implementation
    |-- libFei4 : FE-I4B implementation
    |-- libKU040 : KU040 hw driver
    |-- libNetioHW : FELIX driver
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


