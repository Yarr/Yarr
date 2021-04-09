# RD53B testing with the Single Chip Card

More details and schematics can be found here: [Single Chip Card](https://twiki.cern.ch/twiki/bin/viewauth/RD53/RD53BTesting#RD53B_Single_Chip_Card_SCC)

The information presented here is in addition to the information in the [Chip Manual](https://cds.cern.ch/record/2665301). So please refer to the manual for further details on the chip operation.

## SCC checks before starting

### Components

- ``C24`` should be removed
- Either ``R15`` or ``R17`` should be mounted, not both
- Either ``R14`` or ``R16`` should be mounted, not both
- ``GND_SNS``, ``VDDD_SNS``, ``R_SCAN``, ``R7``, ``R10``, ``R_GND_BDAQ`` should not be mounted
### Jumpers

- Untuned ``IREF_TRIM`` should be set to ``0xB`` (``1011``), meaning only bit 2 closed with a jumper. Ideally the IREF should be trimmed to 4uA.
- If no ChipId jumper is placed, the default ChipId is ``0xF``/``15``.
- ``VDD_EFUSE`` should be shorted to GND (jumper inserted)
- No ``DEBUG`` jumpers (``PIMTM``, ``TEST_MODE``, ``BYP_MODE``) should be placed

### Powering mode

Preferred mode for testing should be LDO mode.

- LDO mode:
    - ``PWR_D`` jumper should have center connected to VIND only
    - ``PWR_A`` jumper should have center connected to VINA only
    - ``VDD_PLL_SEL`` jumper should have center connected to VDDA only
    - ``VDD_CML_SEL`` jumper should have center connected to VDDA only
    - ``SHUNT_EN`` should be open
    - Supply 1.6V on ``VIND`` and ``VINA``

- Direct power:
    - ``PWR_D`` jumper should have center connected to VDDD
    - ``PWD_A`` jumper should have center connected to VDDA
    - If PLL and CML shall be powered externally, connect ``VDD_PLL_SEL`` and ``VDD_CML_SEL`` jumper center connected to VEXT (connecting it to the second Molex connector)
    - ``SHUNT_EN`` should be open
    - Supply ``VDDD`` and ``VDDA`` such that voltage on SCC is not higher than 1.25V, ideally 1.2V.

- SLDO mode:
    - Power jumpers the same as for LDO mode
    - ``SHUNT_EN`` should be closed
    - Crosscheck value of ``R_EXTA``, ``R_EXTD``, ``R_IOFS``, and ``R_IOFS_LB`` to be according to your operational needs (read manual for further info).

## DAQ specifics for RD53B

### Readout Speed

The readout speed that the chip is confgured to has to match the readout speed of the firmware (which is fixed). In order to chanege the readout frequency of the chip one has to change the ``CdrClkSel`` register. These settings correspond to the different readout frequencies (the value is the divider from 1.28Gbps):

- ``0`` : 1280Mbps
- ``1`` : 640Mbps
- ``2`` : 320Mbps
- ``3`` : 160Mbps

Recommended is 640Mbps.

### Number Data Lanes

At the current point in time it is necessary to only one lane (default). This can be chosen via the ``AuroraActiveLanes`` register where each bit represents one lane.

## Scan Console for RD53B

The general structure of the scanConsole command is:
```bash
bin/scanConsole -r configs/controller/specCfg-rd53b.json -c configs/connectivity/example_rd53b_setup.json -s configs/scans/rd53b/<type of scan>.json -p -n 1
```

which specifies the controller (`-r`), the chip list and chip type (`-c`), and the scan (`-s`). The option `-p` selects plotting so plots are produced after the scans.
If you run a scan for the first time, it will create a default configuration for the chip along with running the scan.

Note currently the decoder needs to be limited to one thread ``-n 1``.

## Start-up

### ITkPixV1

ITkPixV1 contains a bug which leads to large current on the digital rail caused by wrongly designed ToT latch. It is not desireable to leave the chip in this high current state for too long without at least passive cooling of some sort. The current can be reduced either by running a ``std_digitalscan`` or the ``clear_tot_mem`` routine (``clear_tot_mem`` might need to be run two times``). Once the current has been reduced it should stay in this mode until fully power cycled.

#### Current

Expected current draw at start-up:

- Digital: typical 1.5A, up to 2.5A
- Analog: 80-100mA

After ``std_digitalscan``:

- Digital: around 600mA
- Analog: around 750mA (depending on config)

### ITkPixV1.1

#### Current

## Tuning Routine

We recommend the following tuning routine:

1. Tune global threshold to 1500e (Note: edge columns need to be adjusted by hand)
2. Tune pixel threshold to 1500e
3. Retune (not changing TDACs) global threshold to 1000e
4. Retune pixel threshold to 1000e


