# YARR: Yet another Rapid Readout

## What is YARR?
YARR is a readout system based around the concept of moving intelligence from the FPGA firmware into the host computer software. The role of the FPGA is the one of a reconfigurable I/O interface and not anymore the one of a hardware accelerator. YARR supports multiple types of FPGA platforms:

* COTS PCIe FPGA cards: CERN SPEC, RelfexCES XpressK7, Trenz TEF1001, and Xilinx KC70
* SLAC's HSIO2/RCE
* ATLAS IBL BOC
* Wuppertal's KU040
* FELIX + NETIO
The currently supported readout chips are:
* FE-I4B
* FE65-P2
* RD53A

## Content
   
* [Software Installation](install.md)
* PCIe Installation
    * [PCIe Kernel Driver Installation](kernel_driver.md)
    * [PCIe Firmware Setup](pcie.md)
* [ScanConsole](scanconsole.md)
    * [FE-I4](fei4.md)
    * [FE65-P2](fe65p2.md)
    * [RD53A](rd53a.md)
* [Troubleshooting](troubleshooting.md)


## Folder Structure
```bash
.
|-- doc : Documentation
|-- eudet : Eudet telescope producer
|   |-- bin
|   |-- build
|   `-- eudaq
|-- gui : QT5 GUI (experimental)
|   |-- YarrGui
|   `-- util
`-- src : Main software dir
    |-- bin
    |-- build
    |-- cmake
    |-- configs : Config templates
    |-- kernel : Custom PCIe kernel driver
    |-- lib
    |-- libBoc : ATLAS IBL BOC hw driver
    |-- libEmu : FE Emulator hw driver
    |-- libFe65p2 : FE65p2 implementation
    |-- libFei4 : FE-I4B implementation
    |-- libKU040 : KU040 hw driver
    |-- libRce : HSIO2 hw driver
    |-- libSpec : PCIe hw driver
    |-- libUtil : Suppert library
    |-- libYarr : YARR core libraries 
    |-- scripts : Scripts to interface with extenral libraries (e.g. ROOT)
    |-- tools : Main executeables
    `-- util : Utility scrips and files
```


