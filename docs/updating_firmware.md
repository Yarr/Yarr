# Guide for switching firmware (v1.3.1 -> v1.4.1)

## Introduction 

Firmware release 1.4.1 includes some major changes to the testing workflow. Firmware release 1.4.1 now also allows to run at a readout speed of 1.28 Gbps, which now is the baseline, replacing the previous baseline readout speed of 640Mbps. Specifically, the firmware includes an update to the deserialiser, which allows the deserialiser sampling delay to be set manually to ensure good data transmission. In order to find the ideal delay setting, an eye diagram has to be run. 

The eye diagram counts the number of non-idle frames over a certain time period, and compares it to the number of expected non-idle frames from the number of service blocks. The link quality is then determined as:

```
LinkQuality = log( 1/(|count  - expected_count|/count))/13.0
```

This link quality factor goes to infinity when the measured count and expected count of non-idle frames match exactly, in which case the link quality factor is set to 1. The eye diagram measures the link quality factor for each deserialiser delay settings. An example of an eye diagram for 4 lanes can be seen here: 

![Example of eye diagram.](images/eye_diagram.png)

The yellow boxes with the “X” indicate good data transmission (over the finite duration of the test). The eye diagram then determines the best deserialiser delay setting by choosing the largest good region and choosing a delay value in the center of the good region. The eye diagram script will then store the best delay value in the spec card controller configuration file.

## Updating the Firmware

Following the instructions in [FPGA Setup](pcie.md), download the latest version of the firmware using the [flash.sh](http://yarr.web.cern.ch/yarr/firmware/flash.sh) script. 

```bash
$ wget --backups=1 http://yarr.web.cern.ch/yarr/firmware/flash.sh
$ <text>
$ ./flash.sh
```

The script will ask you several questions, asking to specify readout speed and and channel configuration. Select 1280 Mbps as the readout speed, and the channel configuration based on your use case. For module QC, 16x1 will most likely be the applicable version, for usage with a 1 DP adapter card. For operation of SCCs, the 4x4 firmware can still be applicable. 


