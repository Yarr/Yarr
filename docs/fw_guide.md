# Firmware Guide

Instructions on how to flash the FPGA can be found [here](https://github.com/Yarr/Yarr-fw/blob/master/syn/xpressk7/README.md).
How to pick your firmware organized by Front-End type, then adapter card.
For the XpressK7 there will always be two version (black), one -**160** and one -**325** version for the different FPGA types. New version of the card (green) is -**trenz**.

## RD53A

### Multi-Module Adapter

- ``bram_rd53a_quad_ohio-XXX/rd53a_quad_ohio_160Mbps.bit`` => Single RD53A channel enabled on Port A
- ``bram_rd53a_quad_ohio-XXX/rd53a_quad_ohio_160Mbps_4chip.bit`` => To enable all four channels.
- ``bram_rd53a_quad_ohio-XXX/rd53a_quad_ohio_160Mbps_2chip2trig.bit`` => To enable two channels for chip readout and two channels for HitOr (to be added).
- In order to power correctly the adapter card, a jumper needs to be added to **3V PCI** pin, as shown on picture below.

![Jumper on FMC Multi-Module Adapter Card ](images/Ohio_jumper.png)

When the board is powered correctly, a red LED should light up. More information about the adapter card can be found [Multi Chip Adapter Card](https://twiki.cern.ch/twiki/bin/viewauth/RD53/RD53ATesting#Multi_Chip_FMC).

On a SCC [Single Chip Card](https://twiki.cern.ch/twiki/bin/viewauth/RD53/RD53ATesting#RD53A_Single_Chip_Card_SCC) the CMD line is AC coupled. On the older Ohio card (before 2019 and serial number < 200) there is additional AC coupling as shown on the picture. This is corrected for the newer Ohio cards from 2019 on with serial number starting from 200.

![Ohio Unmodified CMD ](images/OhioUnmodified_Cmd.png)

To avoid double AC coupling on the CMD line, one should replace the capacitors with jumpers and remove the termination, as shown on the picture.

![Ohio Modified CMD ](images/OhioModified_Cmd.png)

In order to read the HitOrs from Port B and Port D, one has to modify the AC coupling of the data lines on the Ohio card:

![Ohio Unmodified Data ](images/OhioUnmodified_Data.png)

The capacitors should be replaced with jummpers as shown on the picture:

![Ohio Modified Data ](images/OhioModified_Data.png)

