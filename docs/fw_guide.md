# PCIe Firmware and Hardware Guide

Note: For instructions for switching to the 1.28 Gbps firmware, please refer to the [Guide for Updating Firmware](updating_firmware.md)

For the YARR readout, most commonly Series 7 FPGAs are used - specifically the TEF1001 is the recommended FPGA, XpressK7, KC705 and Nereid are also supported. If in doubt of what you have, see the following picture for identification:

![Supported PCIe cards](images/pcie_cards.png)

For more information check the [FPGA board type](#fpga-board-type) section.

## Prerequisites

- Xilinx Vivado Design Suite 2019.2 or latest Vivado Lab Solutions
- A JTAG programmer attached to the JTAG port to the PCIe card
    - We can recommend the [Digilent JTAG HS3](https://www.digikey.com/product-detail/en/digilent-inc/210-299/1286-1047-ND/5015666) or
    - [Digilent JTAG HS2](https://www.digikey.com/en/products/detail/digilent-inc/410-249/3902812) which is also compatible with BDAQ.
- Install the Xilinx cable drivers:
    - Navigate to `/opt/Xilinx/Vivado/2019.2/data/xicom/cable_drivers/lin64/install_script/install_drivers`
    - Install all `install_*` and `setup_*` scripts from the files in this folder:
```
  .
  ├── 52-xilinx-digilent-usb.rules
  ├── 52-xilinx-ftdi-usb.rules
  ├── 52-xilinx-pcusb.rules
  ├── install_digilent.sh
  ├── install_drivers
  ├── setup_pcusb
  └── setup_xilinx_ftdi

```


## Setting up the Xilinx environment and flashing the firmware

Execute the following command to setup the Xilinx environment:
- For Vivado Design Suite
```bash
source /opt/Xilinx/Vivado/2019.2/settings64.sh
```
- For Vivado Lab Solutions
```bash
source /opt/Xilinx/Vivado_Lab/2019.2/settings64.sh
```
- This is the default install path, if the path is not correct you need to point it to your installation.

In order to flash the firmware, download the following script: [flash.sh](http://yarr.web.cern.ch/yarr/firmware/flash.sh)

```bash
$ wget --backups=1 http://yarr.web.cern.ch/yarr/firmware/flash.sh
$ <text>
$ ./flash.sh
```

The script can be used in two ways:

1. Run it by itself (without an argument) and it will ask some questions about what kind of configuration you desire. It will then download the latest firmware and proceed to flash the firmware to the FPGA and PROM.
2. Run it with an already downloaded bit-file as an argument and it will proceed to flash this firmware to the FPGA card. It will still ask what FPGA card you have as this specifies how to flash the firmware exactly.

Largely, case 1 should be applicable for getting the latest official firmware release. Specifically, the script will ask for:

- FPGA card: TEF1001 and XpressK7 are supported in the official releases
- Chip type: YARR is now almost exclusively used for reading out RD53 chips
- FMC card: FMC adapter card attached to the FPGA, most commonly the Ohio card is used
- Readout speed: 1280 Mbps, 640 Mbps and 160 Mbps are supported, and 1280 Mpbs is the baseline readout speed

Please check the [FPGA board type](#fpga-board-type) for more information!

Once you have flashed the firmware **reboot your PC**.

## Check the PCIe Status

- After the system has booted again check that firmware is loaded correctly.
  1. Check that the card appears in ``lspci``
```bash
$ lspci
<Some text>
02:00.0 Signal processing controller: Xilinx Corporation Device 7024
<Possibly more text>
```
  2. Check if the test programs runs successfully (Note that the ``Could not map BAR4, ...`` is normal for the Series 7 FPGAs)
```bash
$ cd Yarr
$ bin/specComTest
void SpecCom::init() -> Opening SPEC with id #0
void SpecCom::init() -> Mapping BARs
void SpecCom::init() -> Mapped BAR0 at 0x0x7f075e4b2000 with size 0x100000
void SpecCom::init() -> Mmap failed
void SpecCom::init() -> Could not map BAR4, this might be OK!
Starting DMA write/read test ...
... writing 8192 byte.g
... read 8192 byte.
Success! No errors.
```

- Your system is now **ready to use**

For further information for the questionnaire see the section below for more information!

## DMA settings, benchmark and stress test

### Controller settings

The SPEC controller reads two optional keys from the ``cfg`` block of the hardware controller config
(for example ``configs/controller/specCfg-rd53b-16x1.json``):

| Key | Default | Meaning |
|-----|---------|---------|
| ``mergeDmaRead`` | ``"auto"`` | ``"auto"``, ``true`` or ``false``. Readout (device to host) DMA can merge physically contiguous pages (hugepages, or an IOMMU) into descriptors of up to ``mergedDescBytes`` instead of one descriptor per 4 KB page. That removes most of the gaps between descriptors and makes the readout faster (about 20 to 40 % on a Nereid). ``"auto"`` merges only if the firmware says that it can take long DMA bursts (see below), otherwise it keeps one descriptor per page. ``true`` and ``false`` force it. |
| ``mergedDescBytes`` | ``32768`` | Largest merged descriptor in bytes, 4096 to 32768 (rounded down to a multiple of 4096). |

```json
"cfg" : {
    "specNum" : 0,
    "mergeDmaRead" : "auto",
    "mergedDescBytes" : 32768
}
```

**Forcing ``mergeDmaRead`` to ``true`` is only safe with firmware that has the fixes of the device-to-host path.**
Firmware without them (every release up to v1.6.0) can lose data with descriptors of more than a few KB and has crashed
test hosts. The controller logs a warning if it is forced on without the capability. Merging only has an effect if the
buffer is contiguous for the card: hugepages, or any buffer behind an IOMMU. Without an IOMMU ordinary buffers rarely
merge, so ``"auto"`` costs nothing there.

The firmware announces the capability in register ``0x1E`` of the control block (``0xC0DE0001``: magic ``C0DE``,
bit 0 = long bursts are safe). Older firmware does not decode that address and answers ``0xDEADBEEF``. At start-up
the controller logs what it found: ``Merged DMA reads (auto): on``, or ``Merged DMA reads (auto): off, firmware lacks
long bursts``.

The firmware also has counters of the device-to-host DMA (TLPs by payload size, cycles busy, waiting for data,
back-pressured by the PCIe side, waiting at a TLP header; registers ``0x8``, ``0xA``..``0xD``, ``0x10``..``0x13``),
which ``specBenchmarkDma`` prints. Descriptors are 32 bytes in host memory so that none crosses a 4 KB page boundary.

### specBenchmarkDma

``specBenchmarkDma -h`` prints the usage:

```
specBenchmarkDma [spec number] [merge 0|1|auto] [hugepage 0|1] [merged descriptor bytes] [verify 0|1|2] [words] [repeats]
```

| Argument | Meaning |
|----------|---------|
| ``spec number`` | Card to use (default 0). |
| ``merge`` | ``1`` merged descriptors, ``0`` one per page, ``auto`` (default) as the firmware allows. |
| ``hugepage`` | ``1``: use a 2 MB aligned hugepage buffer (physically contiguous, so merging has an effect); default ``0``. |
| ``merged descriptor bytes`` | Largest merged descriptor, 4096 to 32768 (default 32768). |
| ``verify`` | ``1``: after the speed benchmark also write a pattern in which every word is unique, read it back and compare every word; ``2``: only that data check. The exit code is 1 if the data differs. |
| ``words``, ``repeats`` | With ``verify``: check only this transfer size (in 32-bit words), ``repeats`` times. |

The first output line says what is used, for example ``merged device-to-host descriptors on (automatic, firmware supports
long bursts, up to 32768 bytes), hugepage buffer``. After the read phase the benchmark prints the TLPs by payload size,
how long the DMA master was active, the payload rate while active and where the active cycles went. The cycle counters are
converted to time with a clock of 320 MHz, so on a build with another clock (the Nereid build runs at 300 MHz) the rates
are off by that ratio; the TLP counts and the percentages are not affected.

```
specBenchmarkDma 1 auto 1          # speed of card 1, hugepage buffer, merging as the firmware allows
specBenchmarkDma 1 0 1             # the same with one descriptor per page, to see what merging gives
specBenchmarkDma 1 1 1 32768 2     # data check only, merged descriptors
```

### specStressDma

Randomised, data-checked DMA test. Each iteration writes a pattern in which every word is unique, reads it back into a
poisoned buffer and compares every word, with random size (8 bytes to 1 MB), host buffer (hugepage or normal pages),
offsets, device address, merging and descriptor size. Optional threads read registers all the time or load the host
memory, to provoke the cases that a quiet host never shows. It stops at the first failure and prints how to repeat it.
Merged descriptors are only used if the firmware supports them.

| Option | Meaning |
|--------|---------|
| ``-s N`` | Card number (default 0). |
| ``-t S`` | Duration in seconds (default 60, 0 = until stopped). |
| ``-r SEED`` | Seed of the random generator (default from the clock, printed). |
| ``-k N`` | Skip N iterations; with the same seed this repeats a failing iteration. |
| ``-n N`` | Stop after N iterations (with ``-k``: repeat exactly one transfer). |
| ``-j N`` | Threads that read registers during the DMA (default 1), ``-p US`` pause between their reads. |
| ``-m N`` | Threads that load the host memory with 256 MB each (default 0). |
| ``-x W`` | Largest transfer in 32-bit words (default 262144 = 1 MB). |
| ``-o W``, ``-b 0\|1``, ``-D`` | Restrict the random parameters: largest host buffer offset (0 = page aligned), normal pages or hugepages only, device address 0 only. |
| ``-a`` | Also use host buffer offsets that are not a multiple of 8 bytes (not known to be supported). |
| ``-q`` | No progress lines. |

The exit code is 0 if everything passed, 1 if the data differs, 2 for a DMA error or a card that does not respond, 3 if
register reads returned wrong values. The progress lines show how much of the time the PCIe side held the DMA back; if it
stays at 0.0 % the host never stalled and the flow control was not exercised (try ``-m``).

```
specStressDma -s 1 -t 600 -j 2          # ten minutes with two threads reading registers
specStressDma -s 1 -r 12345 -k 7 -n 1   # repeat iteration 7 of a failed run with seed 12345
```

**Use it on a test machine.** A DMA that fails can leave the card unresponsive, and the next access to it (even
``lspci`` on the card) can freeze the host, so a reboot is needed before the next test.

## Further information for Questionaire

### FPGA Board Type

#### Trenz TEF1001

The Trenz TEF1001 comes in three revisions which can be identified by the label on the card.
Specific information about the cards can be found on their [wiki](https://wiki.trenz-electronic.de/display/PD/TEF1001+TRM).

##### TEF1001-01 (revision 1)

![TEF1001_R1](images/tef1001_R1.jpg)

Revision 1 specifics:

- Requires to be powered via the PCIe Molex connector.
- Serial numbers 50xxxx -- 52xxxx

##### TEF1001-02 (revision 2)

![TEF1001_R2](images/tef1001_R2.jpg)

Revision 2 specifics:

- Can be powered via PCIe Molex or directly from PCIe.
- DIP switches should be set to `0,0,1,0` (left, left, right, left) from top to bottom, same setting as in the picture, which is ``VCC_IO`` of 1.8V
- Serial numbers 56xxxx -- 86xxxx

##### TEF1001-03 (revision 3)

![TEF1001_R3](images/tef1001_R3.jpg)

Revision 3 specifics:

- Power and DIP switches same as above for rev2
- Uses the same firmware for `tef1001_R2`
- Serial numbers 95xxxx
- The specific produce code we order is [TEF1001-03-B2IX4-K](https://www.trenz-electronic.de/en/PCIe-FMC-Carrier-with-AMD-Kintex-7-160T-DDR3-SODIMM-ECC-GTX-6-6-Gb-s/TEF1001-03-B2IX4-K)

#### PLDA XpressK7

There are two version of the XpressK7 card which differ in the specific FPGA type they use denoted by the 160 and 325. The majority of all people should have the 325 version. The only way to identify in case you do not know which version you have is to connect a JTAG programmer and scan the JTAG chain in Vivado, it will tell you the exact FPGA type. In case you try to flash the wrong firmware version, it will simply fail and tell you that the connected FPGA does not match the one the bit-file was created for.

![xpressk7](images/xpressk7.jpg)

As this card is not produced anymore by PLDA and you require more information, reach out to the ``yarr-user`` mailing list.

#### Xilinx KC705

![KC705](images/kc705.jpg)

#### Numato Nereid (with Kintex-7 XC7K160T)

![Nereid](images/nereid.jpg)

* A jumper has to be placed at the 1V8 pins on the J3 header.

More info can be found [here](https://numato.com/product/nereid-kintex-7-pci-express-fpga-development-board/).

#### CERN SPEC Spartan6

Only used for FE-I4 and FE65-P2 type chips.
![SPEC_S6](images/spec_s6.jpg)

> **Note:** `specS6programFpga` and the `test` binary referenced below have since been removed from the YARR build. This procedure is kept for historical reference only; use the current `specComTest`/`specReadWriteReg` tools in `src/libSpec/app/` for Spartan6 SPEC readiness checks instead.

For the Spartan 6 case it is required to have installed the software first. Then using the software the firmware is loaded into the board:

- Program the FPGA on the SPEC board
```bash
    $ cd Yarr
    $ bin/specS6ProgramFpga <path to Yarr-fw repo>/syn/spec/quad_fei4_revB/quad_fei4_revB.bit
    Opening file: ../hdl/syn/yarr_quad_fei4_revB.bit
    Size: 1.41732 MB
    =========================================
    File info:
    Design Name: yarr.ncd;HW_TIMEOUT=FALSE;UserID=0xFFFFFFFF
    Device:      6slx45tfgg484
    Timestamp:   2015/08/25 12:20:08
    Data size:   1486064
    =========================================
    Reading file.
    Opening Spec device.
    void SpecController::init() -> Opening SPEC with id #0
    void SpecController::init() -> Mapping BARs
    void SpecController::init() -> Mapped BAR0 at 0x0x7f5902cd1000 with size 0x100000
    void SpecController::init() -> Mapped BAR4 at 0x0x7f5903deb000 with size 0x1000
    void SpecController::configure() -> Configuring GN412X
    void SpecController::configure() -> MSI needs to be configured!
    Starting programming ...
    int SpecController::progFpga(const void*, size_t) -> Setting up programming of FPGA
    int SpecController::progFpga(const void*, size_t) -> Starting programming!
    int SpecController::progFpga(const void*, size_t) -> Programming done!!
    int SpecController::progFpga(const void*, size_t) -> FCL IRQ: 0x38
    int SpecController::progFpga(const void*, size_t) -> FCL IRQ indicates CONFIG_DONE
    int SpecController::progFpga(const void*, size_t) -> FCL Status: 0x2c
    int SpecController::progFpga(const void*, size_t) -> FCL STATUS indicates SPRI_DONE
    ... done!
```
- Look for the flags ``CONFIG_DONE`` and ``SPRI_DONE`` to signal successful programming
- Four LEDs on the SPEC board (close to the PCIe connector) should flash up like KITT from Knight Rider
- Which exact bit-file needs to be programmed depends on the use-case
- You can test if the firmware programming was successful by running `bin/specComTest` (see [Check the PCIe Status](#check-the-pcie-status) above). Older documentation referenced a `bin/test` binary that has since been removed:
```bash
$ cd Yarr
$ bin/specComTest
void SpecController::init() -> Opening SPEC with id #0
void SpecController::init() -> Mapping BARs
void SpecController::init() -> Mapped BAR0 at 0x0x7f885e98c000 with size 0x100000
void SpecController::init() -> Mapped BAR4 at 0x0x7f885eab7000 with size 0x1000
void SpecController::configure() -> Configuring GN412X
Starting DMA write/read test ...
... writing 8192 byte.
... read 8192 byte.
Success! No errors.
```
- The firmware has to be loaded after *every* reboot or power cycle of the system!



### FMC card

#### Ohio card

Used for RD53 and STAR chip readout.
![ohio](images/ohio_card.jpg)

In order to power correctly the adapter card, a jumper needs to be added to **3V PCI** pin, as shown on picture below.

![Jumper on FMC Multi-Module Adapter Card ](images/Ohio_jumper.png)

When the board is powered correctly, a red LED should light up. More information about the adapter card can be found [Multi Chip Adapter Card](https://twiki.cern.ch/twiki/bin/viewauth/RD53/RD53ATesting#Multi_Chip_FMC).

##### Modifications

On a SCC [Single Chip Card](https://twiki.cern.ch/twiki/bin/viewauth/RD53/RD53ATesting#RD53A_Single_Chip_Card_SCC) the CMD line is AC coupled.
On the older Ohio card (before 2019 and serial number < 200) there is additional AC coupling as shown on the picture.
This is corrected for the newer Ohio cards from 2019 on with serial number starting from 200.

![Ohio Unmodified CMD ](images/OhioUnmodified_Cmd.png)

To avoid double AC coupling on the CMD line, one should replace the capacitors with jumpers and remove the termination, as shown on the picture.

![Ohio Modified CMD ](images/OhioModified_Cmd.png)

In order to read the HitOrs from Port B and Port D, one has to modify the AC coupling of the data lines on the Ohio card:

![Ohio Unmodified Data ](images/OhioUnmodified_Data.png)

The capacitors should be replaced with jummpers as shown on the picture:

![Ohio Modified Data ](images/OhioModified_Data.png)

#### Creotech 32ch VHDCI

Used for FE-I4 readout.
![vhdci](images/32ch_vhdci.jpg)

### FE Chip Type

Supported chips are:

- FE-I4
- FE65-P2
- RD53A/B
- ABC/HCCStar

### RX Speed

Not all speeds might be supported for all chips this is somewhat chip dependent:

- FE-I4: 160Mbps
- FE65-P2: 160Mbps
- RD53A/B: 160Mbps/640Mbps/1280Mbps
- ABC/HCCStar: 320Mbps

Make sure to change the chip configuration to transmit at the right speed. Details can be found on the respective chip page.

### Channel Configuration

Not all configuration are available for all chip types:

- FE65-P2: 1x1
- RD53A/B: 4x4, 16x1
- ABC/HCCStar: 4x1

If the channel configuration is ZxY then Y is number of lanes per channel (only applies to RD53A/B) and Z is the total number of channels, where the channel typically refers to one chip (or hybrid in case of strips). If one wants to read out 4 RD53A/B single chips: select 4x4. If one wants to read out one RD53A/B quad module with one DP per chip: use 4x4. But if one wants to read out an RD53A/B chip with only 1 DP port: select 16x1 (i.e. 4 channels with 1 lane per DP).
