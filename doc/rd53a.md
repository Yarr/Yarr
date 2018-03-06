# RD53A

## RD53A testing with the Single Chip Card
More details about the SCC [Single Chip Card](https://twiki.cern.ch/twiki/bin/viewauth/RD53/RD53ATesting#RD53A_Single_Chip_Card_SCC)

### Jumper configuration and power on

![Jumper configuration on the SCC ](images/IMG_20180305_162546.jpg)

Default settings for operation in LDO mode

- PWR_A and PWR_D: VINA and VIND (LDO operation)
- VDD_PLL_SEL: VDDA (PLL driver from VDDA supply)
- VDD_CML_SEL: VDDA (CML driver from VDDA supply)
- VREF_ADC (internal ADC voltage reference)
- IREF_IO (internal current refetrence)
- IREF_TRIM: Jumper to 3 to set the internal reference current at 4 μA
- Jumpers JP10 and JP11 should be closed in order to use LANE 2 and 3

**Make sure that the jumper configuration marked in red is correct before powering the chip!!! Applying too high voltage may kill the chip.**

After all jumpers are placed on the SCC, connect the DisplayPort cable to DP1 and power cable to PWR_IN.

Before powering the chip, run the script that turns off the command from the FPGA:

```./bin/rd53a_PowerOn```

Set the power supply to 1.80 V, the current should be around 0.41 A and power on the chip.

Check if the test program runs succesfully:

```./bin/rd53a_test```

(ToDo: add output)


### Digital Scan

```./bin/rd53a_proto_digitalscan```

(ToDo: add output and picture)


### Analog Scan

```./bin/rd53a_proto_analogscan```

(ToDo: add output and picture)


### Scan Console
Coming soon!

