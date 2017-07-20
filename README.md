# Yarr-fw
This firmware is made for the XpressK7 board. This document will explain step by step how to get the firmware working and launch testing programs.

##Connect the board
//Some pictures

##Install Vivado

## Download the source code from the git repository

Firstly, to download the files from the git repository, you need to install git. The prompt will ask your user password.
`$ sudo yum install git`
If this command works the terminal will ask you whether you want to install this package.
`Is this ok [y/d/N]: `
Type yes to confirm you want to install git. 

##Download the source files 

You can download all the source files project.
`$ git clone git@github.com:Yarr/Yarr-fw.git`
Check whether the files were downloaded, by listing the folder. You should see a folder named "Yarr-fw".

##Generate the bitfile

Move in the folder where you will generate the bitfile.
`$ cd Yarr-fw/syn/xpressk7/ddr3_revA/`
To launch the synthesis you just need to launch make.
`$ make`
Prepare a coffee, it will last around 15 minutes to synthesize all the project. 

##Write the bitfile into the flash memory

Move in the folder where the script file are.
`$ cd Yarr-fw/script/`
Launch the script for flashing the memory. At this point you can choose the bitfile by pressing the belonging number. If there is only bitfile you need to press 'y'.
```bash
$ python flash.py
Several bit files found: 
0: /home/***/Yarr-fw/syn/xpressk7/ddr3_revA/yarr.runs/impl_1/top_level.bit
1: /home/***/Yarr-fw/syn/xpressk7/bram_revA/yarr.runs/impl_1/top_level.bit
```
The terminal will ask you if you want to flash the flash memory or the RAM. As you want a persistent system, press F for Flash.
`Will you flash the RAM or the Flash [R/F] ?`
Then shut down the computer. After the next boot firmware is ready to use.
`$ systemctl poweroff`

> **Note:** A soft reboot is not enough to get the system working.

##Core debugging

To activate the debug cores you need to modify a constant in "app.vhd".
`$ vim rtl/kintex7/app.vhd`
At the line 120, you see a constant you can change to "000101". Each bit of this constant belongs to a debug core. You can activate any debug core as you want. Except the MSB bit ("1XXXXX") which belongs to DDR3 IP core user bus which works only if you sythesize the DDR3 version of the firmware.
```VHDL
    constant DEBUG_C : std_logic_vector(5 downto 0) := "000000";
```
Above no debug cores are activated. Below the 1st and 3rd debug cores are activated.
```VHDL
    constant DEBUG_C : std_logic_vector(5 downto 0) := "000101";
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
Debug file found : /home/asautaux/Documents/master/Yarr-fw/syn/xpressk7/bram_revA/yarr.runs/impl_1/debug_nets.ltx
Will you debug with this file [Y/n] ?Y
```
You can launch the debug interace, going into script, then launching the debug script. At this point you can choose the debug file by pressing the belonging number. If there is only debug file you need to press ‘y’. Vivado will be launched displaying the debug interface.


