# Yarr-fw
This firmware is made for the XpressK7 board. This document will explain step by step how to get the firmware working and launch testing programs.

## Connect the board
//Some pictures

## Install Vivado

## Download the source code from the git repository
Firstly, to download the files from the git repository, you need to install git. The prompt will ask your user password.
`$ sudo yum install git`
If this command works the terminal will ask you whether you want to install this package.
`Is this ok [y/d/N]: `
Type yes to confirm you want to install git. 

## Download the source files 
You can download all the source files project.
`$ git clone git@github.com:Yarr/Yarr-fw.git`
Check whether the files were downloaded, by listing the folder. You should see a folder named "Yarr-fw".

## Generate the bitfile
Move in the folder where you will generate the bitfile.
`$ cd Yarr-fw/syn/xpressk7/ddr3_revA/`
To launch the synthesis you just need to launch make.
`$ make`
Prepare a coffee, it will last around 15 minutes to synthesize all the project. 

## Write the bitfile into the flash memory
Move in the folder where the script file are.
`$ cd Yarr-fw/script/`
Launch the script for flashing the memory. At this point you can choose the bitfile by pressing the belonging number. If there is only bitfile you need to press 'y'.
``` bash
$ python flash.py
Several bit files found: 
0: /home/***/Yarr-fw/syn/xpressk7/ddr3_revA/yarr.runs/impl_1/top_level.bit
1: /home/***/Yarr-fw/syn/xpressk7/bram_revA/yarr.runs/impl_1/top_level.bit
```
The terminal will ask you if you want to flash the flash memory or the RAM. As you want a persistent system, press F for Flash.
`Will you flash the RAM or the Flash [R/F] ?`
Then shut down the computer. After the next boot firmware is ready to use.
`$ systemctl poweroff`

>![Attention][attention] **Note:** A soft reboot is not enough to get the system working.

[attention]:http://icons.iconarchive.com/icons/martz90/circle-addon2/24/warning-icon.png
