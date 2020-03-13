# Firmware Guide

## Download and flash Firmware

In order to flash the firmware, download the following script: [flash.sh](http://yarr.web.cern.ch/yarr/firmware/flash.sh)

```bash
wget http://yarr.web.cern.ch/yarr/firmware/flash.sh
```

The script can be used in two ways:
1. Run it by itself (without an argument) and it will ask some questions about what kind of configuration you desire. It will then download the latest firmware and proceed to flash the firmware to the FPGA and PROM.
2. Run it with an already downloaded bit-file as an argument and it will proceed to flash this firmware to the FPGA card. It will still ask what FPGA card you have as this specifies how to flash the firmware exactly.

Once the firmware is flashed please reboot your computer. Also remember that you should power-down all connected FE chips during FPGA programming and that it might be necessary to configure the FPGA once before one can successfully communicate with FE chips (for example by simply running the scanConsole).

## Further information for Questionaire

### FPGA Board Type

todo

### FE Chip Type

todo

### RX Speed

todo

### Channel Configuration

todo
