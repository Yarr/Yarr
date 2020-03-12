#Firmware Identifier Encoding

```
XX-YY-ZZ-VT
```

Where:
- ``XX``: FPGA Board type
- ``YY``: FE Chip type
- ``ZZ``: FMC card type
- ``V``: RX speed
- ``T``: Channel configuration

## FPGA Board Type

- ``00``: unspecified
- ``01``: Trenz TEF1001_R1
- ``02``: Trenz TEF1001_R2
- ``03``: PLDA XpressK7_160
- ``04``: PLDA XpressK7_325
- ``05``: Xilinx KC705
- ``06``: CERN SPEC

## FE Chip Type

- ``01`` : FE-I4
- ``02`` : FE65-P2
- ``03`` : RD53A/ItkPixV1/CROC

## FMC Card Type

- ``01``: Creotech 32CH LVDS (VHDCI)
- ``02``: Ohio Card (DisplayPort)

## RX Speed

- ``1``: 160Mbps
- ``2``: 320Mbps
- ``3``: 640Mbps
- ``4``: 1280Mbps

## Channel Configuration

- ``1``: 4x4 (4 channels each with 4 lanes)
- ``2``: 16x1 (16 channel each one lane)
