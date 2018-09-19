# PCIe FPGA card Installation and Setup

Two kinds of setups have to be distinguished, those that use Series 7 FPGAs (XpressK7, KC705, TEF1001) and those that use the Spartan 6 (CENR SPEC). If in doubt see picture below for identification:

![Supported PCIe cards](images/pcie_cards.png)

## Spartan 6

For the Spartan 6 case it is required to have installed the software first. Then using the software the firmware is loaded into the board:

- Program the FPGA on the SPEC board
```bash
    $ cd Yarr/src
    $ bin/programFpga <path to Yarr-fw repo>/syn/spec/quad_fei4_revB/quad_fei4_revB.bit 
    Opening file: ../hdl/syn/yarr_quad_fei4_revB.bit
    Size: 1.41732 MB
    =========================================
    File info:
    Design Name: yarr.ncd;HW_TIMEOUT=FALSE;UserID=0xFFFFFFFF
    Device:      6slx45tfgg484
    Timestamp:   2015/08/25 12:20:08
    Data size:   1486064
    =========================================
    Reading file.
    Opening Spec device.
    void SpecController::init() -> Opening SPEC with id #0
    void SpecController::init() -> Mapping BARs
    void SpecController::init() -> Mapped BAR0 at 0x0x7f5902cd1000 with size 0x100000
    void SpecController::init() -> Mapped BAR4 at 0x0x7f5903deb000 with size 0x1000
    void SpecController::configure() -> Configuring GN412X
    void SpecController::configure() -> MSI needs to be configured!
    Starting programming ...
    int SpecController::progFpga(const void*, size_t) -> Setting up programming of FPGA
    int SpecController::progFpga(const void*, size_t) -> Starting programming!
    int SpecController::progFpga(const void*, size_t) -> Programming done!!
    int SpecController::progFpga(const void*, size_t) -> FCL IRQ: 0x38
    int SpecController::progFpga(const void*, size_t) -> FCL IRQ indicates CONFIG_DONE
    int SpecController::progFpga(const void*, size_t) -> FCL Status: 0x2c
    int SpecController::progFpga(const void*, size_t) -> FCL STATUS indicates SPRI_DONE
    ... done!
```
- Look for the flags ``CONFIG_DONE`` and ``SPRI_DONE`` to signal successful programming
- Four LEDs on the SPEC board (close to the PCIe connector) should flash up like KITT from Knight Rider
- Which exact bit-file needs to be programmed depends on the use-case
- You can test if the firmware programming was successful by:
```bash
$ cd Yarr/src
$ bin/test 
void SpecController::init() -> Opening SPEC with id #0
void SpecController::init() -> Mapping BARs
void SpecController::init() -> Mapped BAR0 at 0x0x7f885e98c000 with size 0x100000
void SpecController::init() -> Mapped BAR4 at 0x0x7f885eab7000 with size 0x1000
void SpecController::configure() -> Configuring GN412X
Starting DMA write/read test ...
... writing 8192 byte.
... read 8192 byte.
Success! No errors.
```
- The firmware has to be loaded after *every* reboot or power cycle of the system!

## Series 7 FPGA

The FPGA programming for the Series 7 FPGAs is somewhat more complicated, as in this case the FPGA is directly attached to the PCIe bus. This might also lead to some issues discussed at the end of this section.

### Prerequisites

* Xilinx Vivado Design Suite 2016.2 or latest Vivado Lab Solutions
* A JTAG programmer attached to the JTAG port to the PCIe card
  * I can recommend the [Digilent JTAG HS3](https://www.digikey.com/product-detail/en/digilent-inc/210-299/1286-1047-ND/5015666)
* Install the Xilinx cable driver, refer to [UG973](https://www.xilinx.com/support/documentation/sw_manuals/xilinx2016_4/ug973-vivado-release-notes-install-license.pdf)

### Setting up the Xilinx environment

Execute the following command to setup the Xilinx environment:
- For Vivado Design Suite
```bash
source /opt/Xilinx/Vivado/2016.4/settings64.sh
```
- For Vivado Lab Solutions
```bash
source /opt/Xilinx/Vivado_Lab/2016.4/settings64.sh
```
- This is the default install path, if the path is not correct you need to point it to your installation.

### Flashing the Firmware

Details can be found in the [Yarr-FW](https://github.com/Yarr/Yarr-fw/blob/master/syn/xpressk7/README.md). The following will describe the simplest way:

- Execute the ``flash.py`` python script:
```bash
$ cd Yarr-fw/script
$ python flash.py
Several bit files found: 
0: /local/theim/Yarr-fw/syn/spec/fe65p2_revB/yarr.bit
1: /local/theim/Yarr-fw/syn/spec/fe65p2_revB/fe65p2_revB.bit
2: /local/theim/Yarr-fw/syn/spec/fe65p2_revC/yarr.bit
<More files listed>
Choose a file by typing a number:
```
- From the list select the correct firmware you want to program. Generally you have to choose the one specific to your PCIe card, FPGA, and Adapter card. A guide on how to choose firmware can be found [here](fw_guide.md)

- Once you entered the number it will take a few minutes to flash the firmware

- The firmware is written into non-volatile memory and will persist between reboots/power cycles

- After the programming you have to reboot the computer, for instance via:
```bash
$ sudo reboot
```

- After the system has booted again check that firmware is loaded correctly.
  1. Check that the card appears in ``lspci``
```bash
$ lspci
<Some text>
02:00.0 Signal processing controller: Xilinx Corporation Device 7024
<Possibly more text>
```
  2. Check if the test programs runs successfully (Note that the ``Could not map BAR4, ...`` is normal for the Series 7 FPGAs)
```bash
$ cd Yarr/src
$ bin/test 
void SpecCom::init() -> Opening SPEC with id #0
void SpecCom::init() -> Mapping BARs
void SpecCom::init() -> Mapped BAR0 at 0x0x7f075e4b2000 with size 0x100000
void SpecCom::init() -> Mmap failed
void SpecCom::init() -> Could not map BAR4, this might be OK!
Starting DMA write/read test ...
... writing 8192 byte.
... read 8192 byte.
Success! No errors.
```

- Your system is now ready to use

