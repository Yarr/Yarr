# Troubleshooting

The topics in this section are ordered in increasing complexity. If a problem occurs the best way is to start at the basics and crosscheck those first to exclude them. Knowing that these points have been crosschecked will help if further support is needed.

## PCIe Card Troubleshooting

The following points are specific to PCIe cards.

### Check if the PCIe card is identified by the System
- You can check if the card is enumerated by the system with ``lspci``
    - For Series 7 FPGAs
```bash
$ lspci
<Some text>
02:00.0 Signal processing controller: Xilinx Corporation Device 7024
<Possibly more text>
```
    - For the CERN SPEC card
```bash
$ lspci
<Some text>
01:00.0 Non-VGA unclassified device: CERN/ECP/EDU Device 018d (rev 03)
<Possibly more text>
```
- If card does not show up this can have multiple reasons.
    - SPEC card: this means an incompatibility with the motherboard, and the solution would be to switch to another computer.
    - Series 7 FPGA: 
        - If the FPGA is not fully programmed at boot time, the system might not enumerate it. Try performing a 'soft-reboot' where the power is not cut and the FPGA stays programmed.
        - Try reprogramming the firmware in case something went wrong during the firmware setup stage.
        - It has been observed that some machiens seem to be FPGA unfriendly, specifically DELL and HP computers. A list of compatible motherboards can be found [here](compatability.md)

### Check if the DMA transfers works correctly

- You can check the DMA transfer with the PCIe card with ``test`` program
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
- If this program returns any kind of other output, there is a problem with the DMA transfer. This can be caused by multiple things:
    - Corrupt firmware: while it should not happen if there was a problem while the firmware was synthesized. You can try flashing a different firmware, this test does not use the adapter card hence all firmware types should work.
    - It has been observed that there are sometimes hardware and kernel level incompatiblites which cause DMA transfers to fail. Please contact a developer if the previous step did not fix it.

## ScanConsole Troubleshooting

## FE-I4 Troubleshooting

## FE65-P2 Troubleshooting

## RD53A Troubleshooting
