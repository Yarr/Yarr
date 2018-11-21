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

### Chip is not configuring

**Sympton**: Current does not change after a power-cycle when starting a scan and the scan results are empty or not there.

If the test program fails, e.g. the current doesn't change during the test program and there is no output
```
$ ./bin/rd53a_test
void SpecCom::init() -> Opening SPEC with id #0
void SpecCom::init() -> Mapping BARs
void SpecCom::init() -> Mapped BAR0 at 0x0x7f45464ee000 with size 0x100000
void SpecCom::init() -> Mmap failed
void SpecCom::init() -> Could not map BAR4, this might be OK!
>>> Configuring chip with default config ...
... done.
>>> Checking link status: 0x1
All links are synced!
>>> Trigger test:
Trigger: 1
Trigger: 2
Trigger: 3
Trigger: 4
Trigger: 5
Trigger: 6
Trigger: 7
Trigger: 8
Trigger: 9
Trigger: 10
Trigger: 11
Trigger: 12
Trigger: 13
Trigger: 14
Trigger: 15
>>> Enabling digital injection
>>> Enabling some pixels
>>> Digital inject test:
```
**Resolve by:**

- Try power-cycling the chip.
- Make sure the DP cable is plugged into the right ports and you have selected the correct Tx/Rx links in the connectivity.
- Check that the power is 1.80 V and the current is above 0.41 A, if this is not the case check your power cable.
- Test a different possibly shorter DisplayPort cable.
- Meausure the analog regulator output voltage, if below 1.1V consider installing a Vref hack (ask experts).
- If all above fails: try operating in direct powering.

![Jumper configuration for **direct powering** on the SCC ](images/IMG_20180305_170121.jpg)

Jumper configuration for **direct powering**

- PWR_A and PWR_D: VDDA and VDDD (direct powering)
- VDD_PLL_SEL: VDDA (PLL driver from VDDA supply)
- VDD_CML_SEL: VDDA (CML driver from VDDA supply)
- VREF_ADC (internal ADC voltage reference)
- IREF_IO (internal current refetrence)
- IREF_TRIM: Jumper to 3 to set the internal reference current at 4 μA
- Set the power supply to 1.30 V, the current should be 0.41 A
- Turn off the command from the FPGA and turn on the power supply

**Make sure that the jumper configuration marked in red is correct before powering the chip!!! <span style="color:red"> Do not apply higher voltage than 1.30 V</span>. Applying too high voltage may kill the chip.**

### Readout errors

**Sympton:** You see a lot of readout errors during a scan, like this

```
16848 [0] Received data not valid: [4245,16848] = 0x3dee006f 
16848 [0] Received data not valid: [4246,16848] = 0xc58f9fff 
16848 [0] Received data not valid: [4250,16848] = 0xe51ff9ff 
16848 [0] Received data not valid: [4959,16848] = 0x9dbfffd9 
```

**Resolve by:**

- Increase or decrease the ``SldoTrimAna`` and ``SldoTrimDig`` register (try going in steps by 5) or tune them to output 1.2V
- Increase or decrease the ``CmlTapBias0`` register (try testing in steps of 100)
- Try a different kind of DisplayPort cable (typically short is better)

### Problem with the digital scan

**Sympton:** The digital scan looks blocky (see picture)

![Digital scan example](images/rd53a_proto_digital_Occupancy.png)

**Resolve by:** check that aurora lines are connected and running. The jumpers JP10 and JP11 on the SCC have to be closed in order to use LANE 2 and LANE 3.

### Endless readout loop or data corruption

**Sympton:**

- when running `./bin/rd53a_test` the readout loop is never ending
- When running a scan there are `Data not valid` mentioned in the log

**Resolve by:**

- Try increasing/decreasing the analog/digital voltage trim settings (`SldoAnalogTrim` and `SldoDigitalTrim`).
- Try a different possibly shorter DisplayPort cable
- Make sure the scan you are running does not put the chip under very high load (e.g. noise scan with too low threshold).

### Noise Scan is empty

**Sympton:**

- Noise scan (or similar) has 0 hits

**Resolve by:**

- Check that your enable mask is not all `0`

