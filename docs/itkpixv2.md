# ITkPixV2 testing with the Single Chip Card

More details and schematics can be found here: [Single Chip Card](https://twiki.cern.ch/twiki/bin/viewauth/RD53/RD53BTesting#RD53B_Single_Chip_Card_SCC)
TODO update for itkpixv2? ^

The information presented here is in addition to the information in the [Chip Manual](https://cds.cern.ch/record/2665301). So please refer to the manual for further details on the chip operation.

## SCC checks before starting

### Components

- ``C24`` should be removed
- Either ``R15`` or ``R17`` should be mounted, not both
- Either ``R14`` or ``R16`` should be mounted, not both
- ``GND_SNS``, ``VDDD_SNS``, ``R_SCAN``, ``R7``, ``R10``, ``R_GND_BDAQ`` should not be mounted
### Jumpers

- IREF should be trimmed to 4uA. The 4 ``IREF_TRIM`` pin pairs correspond to a binary number that tunes the IREF. To tune IREF, various ``IREF_TRIM`` jumper configurations are tried until the ``Vref_ADC`` pin reads as close to ``0.845 V`` as possible. Untuned ``IREF_TRIM`` should be set to ``0xB`` (``1011``), meaning only bit 2 closed with a jumper. 
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
    - ``SHUNT_EN`` jumper should be closed
    - ``VOFS`` jumper should be closed
    - Crosscheck value of ``R_EXTA``, ``R_EXTD``, ``R_IOFS``, and ``R_IOFS_LB`` to be according to your operational needs (read manual for further info).
    - Supply current fitting your offset and slope choice, optimal voltage across the module is around 1.6V

## DAQ specifics for ITkPixV2

### Number Data Lanes

Choose the number of active data lanes according to your setup and firmware. This can be chosen via the ``AuroraActiveLanes`` register where each bit represents one lane.
Typically 16x1 firmware uses one lane ``AuroraActiveLanes = 1`` and 4x4 firmware uses ``AuroraActiveLanes = 15``.

## Scan Console for ITkPixV2

The general structure of the scanConsole command is:
```bash
./bin/scanConsole -r configs/controller/specCfg-itkpixv2.json -c configs/connectivity/example_itkpixv2_setup.json -s configs/scans/itkpixv2/<type of scan>.json -p
```

which specifies the controller (`-r`), the chip list and chip type (`-c`), and the scan (`-s`). The option `-p` selects plotting so plots are produced after the scans.
If you run a scan for the first time, it will create a default configuration for the chip along with running the scan.

## Start-up

#### Current

Expected current draw at start-up:

- Digital: 200-250mA
- Analog: 80-100mA

After ``std_digitalscan`` (depends on exact config):

- Digital: around 600m
- Analog: around 750mA

## Tuning Routine

We recommend the following tuning routine:

1. Tune global threshold to 1200e
2. Tune global preamp to 7 ToT @ 6000e
3. Tune global threshold to 1200e
4. Tune pixel threshold to 1000e

## Active Lanes

ITkPix can be configured to be read out via 1 and up to 4 lanes. Typically for a chip mounted onto a SCC you would want to use 4 lane read out, while a chip inside a quad module might only use 1 lane.

Three registers are involved in configuring how many lanes should be used for the read out:

- ``AuroraActiveLanes``: determines how many lanes are used to transmit data (does not disable the physical link), possible values 1,3,7,15 to select 1, 2, 3, and 4 lane readout.
- ``DataMergeOutMux0/1/2/3``: selects which physical lane a logical lane will be transmitted on (allows us to re-reoute data in case the hardware wiring does not match the nominal lane order). Possible values 0, 1, 2, 3.
- ``SerLaneEn``: eneables/disables the serializer in a physical lane (0 to enable, 1 to disable)

When using the YARR-PCIe cards and SCC, possible values are:

| Number of Lanes | ``AuroraActiveLanes`` | ``SerLaneEn`` | ``DataMergeOutMux0/1/2/3`` |
| ----- | --------- | ----------- | --------------- |
| 4 | 15 | 15 | 3, 2, 1, 0 |
| 3 | 7 | 14 | 3, 2, 1, 0 |
| 2 | 3 | 12 | 3, 2, 1, 0 |
| 1 | 1 | 8 | 3, 2, 1, 0 |

Please note that the number of active lanes might also need to be specified in the controller config to inform the firmware about how many lanes are used for the readout.

## Link sharing

TODO

# Testing with ITkPixV2 Quad Modules

TODO

## Testing with ITkPixV2

Tuning routine:
- std_digitalscan
- std_analogscan
- std_tune_globalthreshold (target 1200e)
- std_tune_globalpreamp (target 7 ToT at 6000e)
- std_tune_globalthreshold (target 1200e)
- std_tune_pixelthreshold (target 1000e)
- std_thresholdscan
- std_totscan (target 6000e)


## Configuration files with 1-DisplayPort Data Adapter Card

The DisplayPort is connected to Port A of the Ohio cars. Note that DisplayPort pins are connected to:
- pins 1,3 correspond to channel 0
- pins 4,6 to channel 1
- pins 7,9 to channel 2
- pins 10,12 to channel 3

Connectivity file when DisplayPort cable is connected to Port A of the Ohio card

```bash
"chipType" : "ITKPIXV2",
"chips" : [
    {
        "config" : "configs/itkpixv2_1DPQuad04_Chip1.json",
        "tx" : 0,
        "rx" : 2,
        "enable" : 1,
        "locked" : 0
    },
    {
        "config" : "configs/itkpixv2_1DPQuad04_Chip2.json",
        "tx" : 0,
        "rx" : 1,
        "enable" : 0,
        "locked" : 0
    },
    {
        "config" : "configs/itkpixv2_1DPQuad04_Chip3.json",
        "tx" : 0,
        "rx" : 0,
        "enable" : 0,
        "locked" : 0
    },
    {
        "config" : "configs/itkpixv2_1DPQuad04_Chip4.json",
        "tx" : 0,
        "rx" : 3,
        "enable" : 0,
        "locked" : 0
    }
]
}
```

Summary table of Chip configs:

| #Chip | `ChipID` | `DataMergeOutMux0/1/2/3` | `SerEnLane` | 
| :---: | :---: | :---: | :---: |
| Chip1 | 12 | 2/3/0/1 | 4 |
| Chip2 | 13 | 0/1/2/3 | 1 |
| Chip3 | 14 | 1/2/3/0 | 8 |
| Chip4 | 15 | 0/1/2/3 | 1 |
