# Firmware Guide

Instructions on how to flash the FPGA can be found [here](https://github.com/Yarr/Yarr-fw/blob/master/syn/xpressk7/README.md).
How to pick your firmware organized by Front-End type, then adapter card.
For the XpressK7 there will always be two version (black), one -**160** and one -**325** version for the different FPGA types. New version of the card (green) is -**trenz**.

## RD53A

### Multi-Module Adapter

- ``bram_rd53a_quad_ohio-XXX/rd53a_quad_ohio_160Mbps.bit`` => Single RD53A channel enabled on Port A
- ``bram_rd53a_quad_ohio-XXX/rd53a_quad_ohio_160Mbps_4chip.bit`` => To enable all four channels.
- ``bram_rd53a_quad_ohio-XXX/rd53a_quad_ohio_160Mbps_2chip2trig.bit`` => To enable two channels for chip readout and two channels for HitOr (to be added).

