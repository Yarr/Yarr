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

>![Attention][attention] **Note:** A soft reboot is not enough to get the system working.

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


[attention]:data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAEo0lEQVR42o1We0xbZRT/zr23L9qCAZdBOgl1YSCBZBkBYcY/tiiZ8x+nKJiwiZooBOMrcdZlajQKuGRKzMx0jpkQmEMhBKPA8DVfyOaSuTAyksXYMXl2Ikqh7X19nu9rb+lzepL2np6e75zz/c7rArkBvVBkc1ffbKpzmmGnAFAKQDYwOaXEp1N6aVWh34xfV3rbpwK/p7MBqYQHb7MV3LnR3G6WhFpRIAKgdTRO8QPcPuVOCNUpaDrRZFXvG/cpnlcn17z/6eBklbMh1y4eNYngEES0yKymDCPiBL90jRJFo37fmtZc9/NKd1oH/dud+3NsYrtkAsKDxqjDGswFM2c8o8eBeWAShAxUhdLloOa576eVQ0kOuisdDS6n1GWS0LgA7B8OR0QJWLDGM+bw+m8MRUcvikrJgl/dV3/W3x118GKhtaDGZZkwm8EhcuOpMQEQ+Amq62kx03RKZJn6v5+Xy16bCni5paE7nKfsVuEhSRJY3DQxOn4fUaSlh74AwWKlE8/VEF2Rk3Q4j1+qqsNaSO+958eVenjKbXHvybdcMZsFpFiM4/mNuxvp5qffBob41eOvkNn+I0k6Bo8XBFnWteGZUCH0lNs9rkyxDaNPWy2iI4ts6zxPpKwcostBQlWZXHisgijLvrTVhbcgPr/2EgxVO0YRnrtECYycGEmM8u6mVsjb00z//G4A5j77kJYeHiILw13kt45nUuozXlMpXQ3qX8HIdseszSLkCix/SOGiW+cz8reQrcfGAESJXv+2H+YGj9GyjtMs0+Riyw7wX7kYp2/AxSoqGNLn4XS1XbFaBJGXZooblLR+SrJvr+GHVybHYXbgA1p08COu8M/EGEw8v5tGTsSdjTjQYKSKO5AiDuIIDZOS1k94NzMK+WYwuUcRsjeiOlOvNxLfmYGks+iAoAMVhirtszZrGKLYSgDJDOWdY9R2S+H69XUdls6O0uzqXcSAJTg/Tc83VhA9FIyrKHaDQBAhGizPGHXYhLtFMf4Gmx5sIZtb2uJkyt9LJDjnJc7ibXFy74k3ydWut+JkGs4nTPKX0Flq9eRniq0Sr6LwDcw3bSCVPRdAtGfGJfDaxx0wP9xDK7p+IbFyPbRGzu0th9DiTDQHKlbRvF87AI+7TO5alxkbLTKU0cGWZw8T1/1PJFUU1VTQAqtUwr5IrJyFkZNwua2JRvoAG41qn8/JhRyXvq22U5k2sY7BxATFB94nebseTjuTUtHimUEy+fJeXkYMnpWA1vvAr4F6bqF5k6ng3lzThMUsOPhiYdGZcGaDkNhEsaW4LseQqSJDZD2QkKyvfr2olL0zrXijIR4ptjTc6hS7WC6EcDqAGI1j8LEykqDDKgd5xJ5M+7V9TZdD6+PaoOMllv15GbhwJAA+9/4vQuGFQ1hiFwO659HJYPLCMejdIktDAVuZEq5Mge/haHWx5UkTNhqvIlwPuGhWr+HKbJkKpV+ZBj3pMhXsyJba7SaoxcSzJg+vz5gchNckDjWNavh20ffDX6rnvT8Ub6KtG4LwSJ7krsoS63LMwk6c5qUIG39tQTh8OI0vLeFry7llrffEnJL2teVfUUJlaN+sjcQAAAAASUVORK5CYII=

