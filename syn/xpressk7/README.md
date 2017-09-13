# XpressK7
The XpressK7 card requires an external programmer to be connected via the JTAG connector.
Because of this you will need an installation of Xilinx Vivado (or at least the Xilinx programming software) and source their script `$ source /opt/Xilinx/Vivado/2016.2/settings64.sh`

## Write the bitfile into the PROM memory and the FPGA

To get the board with the firmware working inside, the FPGA has a volatile. So, to save the firmware till the next reconfiguration, the firmware have to be written in a non-volatile memory which is outside of the FPGA.

Two techniques are shwown in this domcument to flash the PROM and the FPGA. The first one shows how to pertorm that with Vivado and the second with a script made to simplify this operation.

### Using the Vivado GUI

#### Step 1: Open the hadware manager
First open Vivavo and then the Open Hardware Manger.

![Image not availabe](https://raw.githubusercontent.com/Yarr/Yarr-fw/master/doc/flash_step_1.png)

#### Step 2: Memory Configuration file generation
A memory configuration file has to be generated. So, click "Generate Memory Configuration File".

![Image not availabe](https://raw.githubusercontent.com/Yarr/Yarr-fw/master/doc/flash_step_2.png)

A window will appear with a form to fill out. Fill the forms and chek the box as if it's shown in the the list and picture below.

* Format: MCS (To distinguish it from the bitfile)
* Memory Part: 28f00ap30b-bpi-x16
* Filename: Where you want to create the output memory configuration file. To choose press the trip point button
* Interface: BPIx16
* Load bistream files: checked
 * Start Adress : 0000000 (default option)
 * Direction : up (default option)
 * Bitfile : Click on the triple point button on the right and choose a bitfile.
 * Load data file : unchecked (default option)
 * Write checksum : checked (optional)
 * Disable bit swapping : unchecked (default)
 * Overwrite : depending if you want to overwrite an existing memory configuration file

As soon as you have done this, clik Ok to generate the file.

![Image not availabe](https://raw.githubusercontent.com/Yarr/Yarr-fw/master/doc/flash_step_3.png)

#### Step 3: Write the firmware into the PROM
Connect the board to the JTAG debugger.

![Image not availabe](https://raw.githubusercontent.com/Yarr/Yarr-fw/master/doc/flash_step_4.png)

Vivado needs information about the flash memory.

![Image not availabe](https://raw.githubusercontent.com/Yarr/Yarr-fw/master/doc/flash_step_6.png)

Choose again the 28f00ap30b-bpi-x16 memory.

![Image not availabe](https://raw.githubusercontent.com/Yarr/Yarr-fw/master/doc/flash_step_7.png)

Click ok to perform to program the PROM memory.

![Image not availabe](https://raw.githubusercontent.com/Yarr/Yarr-fw/master/doc/flash_step_8.png)

Fill out the window with the same previous informations. The configuration file and PRM file were generated previously at the Memory Configuration File generation step. When you have done, click on the ok button.

![Image not availabe](https://raw.githubusercontent.com/Yarr/Yarr-fw/master/doc/flash_step_9.png)

A window wil show the progress. It will last a while.

![Image not availabe](https://raw.githubusercontent.com/Yarr/Yarr-fw/master/doc/flash_step_10.png)

![Image not availabe](https://raw.githubusercontent.com/Yarr/Yarr-fw/master/doc/flash_step_11.png)

#### Step 4: Tranfer the firmware to the FPGA
Two solutions are available to transfer the firmware to the FPGA. First you can simply press the configuration button, to tranfer that from the PROM.!

[Image not available](https://raw.githubusercontent.com/Yarr/Yarr-fw/master/doc/board_board_configuration_components.jpg)

To see the debug, you need to refresh the device.

![Image not availabe](https://raw.githubusercontent.com/Yarr/Yarr-fw/master/doc/flash_step_12.png)

Then you get an window like the one below.

![Image not availabe](https://raw.githubusercontent.com/Yarr/Yarr-fw/master/doc/flash_step_13.png)


If you don't have an easy acces to the borad, you can program the FPGA from Vivado. It will send the bitfile from the JTAG programmer directly to the FPGA

![Image not availabe](https://raw.githubusercontent.com/Yarr/Yarr-fw/master/doc/flash_step_5.png)


### Using the python script

A python script was created to simplify the FPGA configuration.
#### Step 1: Laucnch the script
Move in the folder where the script file are.
`$ cd Yarr-fw/script/`
Launch the script for flashing the memory. At this point you can choose the bitfile by pressing the belonging number. If there is only bitfile you need to press 'y'.
```bash
$ python flash.py
Several bit files found: 
0: /home/***/Yarr-fw/syn/xpressk7/ddr3_revA/yarr.runs/impl_1/top_level.bit
1: /home/***/Yarr-fw/syn/xpressk7/bram_revA/yarr.runs/impl_1/top_level.bit
```

#### Step 2: Check if the firmware is running

![Image not availabe](https://raw.githubusercontent.com/Yarr/Yarr-fw/master/doc/board_on_board_leds.jpg)

The LEDs on the board should blink. If they don't, check the FMC/JTAG switch, press the configuration push button.

![Image not availabe](https://raw.githubusercontent.com/Yarr/Yarr-fw/master/doc/board_board_configuration_components.jpg)

Once the LEDs are blinking, reboot the computer. After the next boot firmware is ready to use.
`$ sudo reboot`


You can check if the PCIe communication works by typing the command below.
```bash
$ lspci | grep 7024
01:00.0 Signal processing controller: Xilinx Corporation Device 7024
```

## Generate the bitfile
Move in the folder where you will generate the bitfile.
> Before generating any bitfile, generate the ddr3_octa_fei4_revA-160 version bitfile. Otherwise You would get errors because the IP are designed for the FPGA xc7k160. Then you can generate any bitfile.

`$ cd Yarr-fw/syn/xpressk7/bram_octa_fei4_revA-160`
To launch the synthesis you just need to launch make.
`$ make`
Prepare a coffee, it will last around 15 minutes to synthesize all the project. 

## Core debugging
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
