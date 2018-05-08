# Firmware Guide

How to pick your firmware organized by Front-End type, then adapter card.
For the XpressK7 there will always be two version, one -**160** and one -**325** version for the different FPGA types.

## RD53A

### Multi-Module Adapter

- ``bram_rd53a_quad_ohio-XXX/rd53a_quad_ohio_160Mbps.bit`` => Single RD53A channel enabled on Port A
- In order to power correctly the adapter card, a jumper needs to be added to **3V PCI** pin, as shown on picture below.

![Jumper on FMC Multi-Module Adapter Card ](images/Ohio_jumper.png)

When the board is powered correctly, a red LED should light up. More information about the adapter card can be found [Multi Chip Adapter Card](https://twiki.cern.ch/twiki/bin/viewauth/RD53/RD53ATesting#Multi_Chip_FMC).
