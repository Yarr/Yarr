# Yarr-fw
This firmware is made for the XpressK7 and the SPEC board. This document will explain step by step how to get the firmware working and launch testing programs.

## Cores docs

* Wishbone-Express core : (https://github.com/Yarr/Yarr-fw/blob/master/rtl/kintex7/wbexp-core/README.md)
* DDR3K7 core : (https://github.com/Yarr/Yarr-fw/tree/master/rtl/kintex7/ddr3k7-core/README.md)

## SPEC
Firmware bit-files for the SPEC card can be found in `syn/spec/`

### Flahing the SPEC card
You can use the tool supplied with the YARR sw (https://github.com/Yarr/Yarr):
```bash
$ bin/program path/to/file/spec.bit
```

## XpressK7
The XpressK7 card requires an external programmer to be connected via the JTAG connector.
Because of this you will need an installation of Xilinx Vivado (or at least the Xilinx programming software) and source their script `$ source /opt/Xilinx/Vivado/2016.2/settings64.sh`

### Write the bitfile into the flash memory and the FPGA
Move in the folder where the script file are.
`$ cd Yarr-fw/script/`
Launch the script for flashing the memory. At this point you can choose the bitfile by pressing the belonging number. If there is only bitfile you need to press 'y'.
```bash
$ python flash.py
Several bit files found: 
0: /home/***/Yarr-fw/syn/xpressk7/ddr3_revA/yarr.runs/impl_1/top_level.bit
1: /home/***/Yarr-fw/syn/xpressk7/bram_revA/yarr.runs/impl_1/top_level.bit
```
![Image not available](https://raw.githubusercontent.com/Yarr/Yarr-fw/master/doc/board_on_board_leds.jpg)

The LEDs on the board should blink. If they don't, check the FMC/JTAG switch, press the configuration push button.

![Image not available](https://raw.githubusercontent.com/Yarr/Yarr-fw/master/doc/board_board_configuration_components.jpg)

Once the LEDs are blinking, reboot the computer. After the next boot firmware is ready to use.
`$ sudo reboot`


You can check if the PCIe communication works by typing the command below.
```bash
$ lspci | grep 7024
01:00.0 Signal processing controller: Xilinx Corporation Device 7024
```

### Generate the bitfile
Move in the folder where you will generate the bitfile.
> Before generating any bitfile, generate the ddr3_octa_fei4_revA-160 version bitfile. Otherwise You would get errors because the IP are designed for the FPGA xc7k160. Then you can generate any bitfile.

`$ cd Yarr-fw/syn/xpressk7/bram_octa_fei4_revA-160`
To launch the synthesis you just need to launch make.
`$ make`
Prepare a coffee, it will last around 15 minutes to synthesize all the project. 

### Core debugging
To activate the debug cores you need to modify a constant in "bram_yarr.vhd" or "ddr3_yarr.vhd".
`$ vim rtl/kintex7/app.vhd`
At the line 432, you see a constant you can change to "0110". Each bit of this constant belongs to a debug core. You can activate any debug core as you want. Except the MSB bit ("1XXX") which belongs to DDR3 IP core user bus which works only if you sythesize the DDR3 version of the firmware.
```VHDL
      app_0:app
      Generic map(
        DEBUG_C => "0000", 
        address_mask_c => X"000FFFFF",
        DMA_MEMORY_SELECTED => "BRAM" -- DDR3, BRAM 
        )

```
Above no debug cores are activated. Below the 1st and 3rd debug cores are activated.
```VHDL
      app_0:app
      Generic map(
        DEBUG_C => "0110", 
        address_mask_c => X"000FFFFF",
        DMA_MEMORY_SELECTED => "BRAM" -- DDR3, BRAM 
        )

```
After modifying, launch the synthesis. It will last a while.
`$ cd syn/xpressk7/bram_revA/`
`$ make clean`
`$ make`

Once the synthesis you need to write the bitfile to the FPGA as described in the "Write the bitfile into the flash memory". After rebooting the pc you can launch the Vivado debug interface
by launching the python script in the "script" folder.
```bash
$ cd script
$ python debug.py 
Debug file found : /home/asautaux/Documents/master/Yarr-fw/syn/xpressk7/bram_quad_fei4_revA-325/debug_nets.ltx
Will you debug with this file [Y/n] ?Y
```
You can launch the debug interace, going into script, then launching the debug script. At this point you can choose the debug file by pressing the belonging number. If there is only debug file you need to press ‘y’. Vivado will be launched displaying the debug interface.


