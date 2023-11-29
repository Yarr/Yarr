# Star testing

## Test program

There is preliminary support for the ABC and HCC star ASICs (Front-end for
Strips). Currently only initial testing has been done.
The test program writes an HCC register, and reads back data packets
(HPR will be sent back continuously, once every 1ms).

Using default SPEC hardware:

```bash
bin/star_test
```

Or by specifying a config for communication with felix_core:

```bash
bin/test_star configs/controller/netio.json
```

For comparison purposes, a barebones interface to the itsdaq FW is provided:

```bash
bin/test_star configs/controller/itsdaq.json
```

The emulator version can be run as follows:

```bash
bin/test_star -r 0 configs/controller/emuCfg_star.json
```

## Loop Actions

List of available loop actions and their configuration parameters.

### StarTriggerLoop

The Star version of the trigger loop is similar to other variants.
This will repeatedly send a particular command to the front end. 

All config parameters are optional, see defaults:
    
- l0_latency ``<int>``: time in bunch crossings between injection and trigger (default 45)
- noInject ``<bool>``: if false, inject calibration charge. if true, no injection (default false)
- digital ``<bool>``: if true inject digital pulse instead of calibration pulse (default false)
- trig_count ``<int>``: number of injections, if 0 will run for specified time (trig_time) (default 50)
- trig_frequency ``<int>``: trigger(/injection)frequency in Hz (default 1000)
- trig_time ``<int>``: time in seconds, if count is set to 0 will run for this amount of time (default 10)

### StarMaskLoop

The Star version of the mask loop allows for iteration over various mask
settings. There are a few different modes depending on how the flags are set.

The standard mode loops over a set of strips and allows charge injection to
all enabled strips. The number of strips is given by the range of the scan
(min to max).

A second mode allows for cross-talk measurements. This sets the mask and
calibration mask registers differently. All strips in a group will be enabled,
but charge injection is enabled on a smaller number of strips.

Finally, in nmask mode, the bin number corresponds to the number of channels
that are enabled, this makes little sense except with the parameter flag set.

Required config parameters:
    
- min ``<int>``: start bin
- max ``<int>``: end bin
- step ``<int>``: step size through possible bins
- nMaskedStripsPerGroup ``<int>``: number of strips that are masked in each group
- nEnabledStripsPerGroup ``<int>``: number of strips with charge injection enabled in each group
- EnabledMaskedShift ``<int>``: offset of the enabled strips with the group

Optional config parameters:

- parameter ``<bool>``: if true this identifies as a parameter loop so that data from each step will be placed in a different bin (default: mask loop)
- maskOnly ``<bool>``: if true only the mask register is written, the calibration register remains unchanged (default: false)
- doNmask ``<bool>``: if true run in nmask mode (default: false)

## Configuration parameters

Both StdParameterLoop and the list of parameters in the prescan field of the
scan configuration use the same names for parameters. These are formatted as
follows.

Most of the parameters are named directly from register fields in the ASIC
documentation. A loop over a field in the ABC is named ABCs_{NAME}, and one
in the HCC is named HCC_{NAME}.

Some of the more commonly used ones:

- ABCs_STR_DEL: strobe delay
- ABCs_BVT: discriminator threshold
- ABCs_BTRANGE: trim range
- HCC_CFD_PRLP_FINEDELAY: fine delay of the PRLP output from HCC

A special virtual register is ABCs_MASKs. If this is set to 1, all masks are
enabled (resulting in 0 outpu), otherwise all masks are disabled.

## Scan Console

The general structure of the Scan Console commands is:

```bash
bin/scanConsole -r configs/controller/itsdaq.json -c configs/connectivity/daqload_setup.json -s configs/scans/star/<type of scan>.json -p
```
which specifies the controller (`-r`), the chip list and chip type (`-c`), and the scan (`-s`). The option `-p` selects plotting so plots are produced after the scans.

### Scans from electrical QC

A description of the electrical QC procedure can be found [here](https://docs.google.com/document/d/13OeSVeLvmdswC8ipPiWd7VqQg61dtnMN6De9_zk1-YU/edit#heading=h.1sa1dkhjrwds).

Examples of the results and how to run different scans is shown below.

### Pedestal Trim

To run a pedestal trim tuning execute the following command:
```bash
bin/scanConsole -r configs/controller/itsdaq.json -c configs/connectivity/daqload_setup.json -s configs/scans/star/std_tune_trim_at_pedestal.json -p 
```
An example of occupancy map after a successful trim tuning for a hybrid module is given below.
![Occupancy map Trim Tuning](images/MGF_star_trim_OccupancyMap-14.png)

### Strobe Delay 

To run a strobe delay scan execute the following command:
```bash
bin/scanConsole -r configs/controller/itsdaq.json -c configs/connectivity/daqload_setup.json -s configs/scans/star/std_strobe_delay.json -p 
```
An example of the occupancy map after a successful strobe delay scan for a hybrid module for which the optimal strobe delay value is 21 is given below.
![Occupancy map Strobe Delay](images/MGF_star_strobedelay_OccupancyMap-21.png)

The strobe delay scan can also be run on a full module, instead of just a hybrid. An example of the occupancy map, as well as the strobe delay (``ABCs_STR_DEL``) map after a successful strobe delay scan for a full module is given below.
![Occupancy map Strobe Delay, Full Module](images/MGF_star_fullmodule_strobedelay_OccupancyMap-20.png)
![Strobe Delay map, Full Module](images/MGF_star_fullmodule_strobedelay_ABCs_STR_DEL_Map.png)



### Three/N Point Gain / Response curve

To run a three point gain scan execute the following command:
```bash
bin/scanConsole -r configs/controller/itsdaq.json -c configs/connectivity/daqload_setup.json -s configs/scans/star/std_npointscan.json -p
```

Config parameters for ``ABCs_BCAL``:  
- max <int>: maximum value of ABCs_BCAL
- min <int>: minimum value of ABCs_BCAL
- step <int>: step size of ABCs_BCAL
The value of ``ABCs_STR_DEL`` has to be set to the correct value from the Strobe Delay scan. In case an N-point gain scan (i.e. Response Curve) is needed, the step size can be decreased to sample more finely on parameter ``ABCs_BCAL``. 
The threshold and noise mean and dispersion value (for everything scanned) will be given in the output of the code, for example:
```text
[0] Threashold Mean = 67.5668 +- 0.579076
[0] Noise Mean = 1.65438 +- 0.422169
```
Example of the threshold and noise map, as well as the response curve for a hybrid module are given below:
![Threshold map NPointGain](images/MGF_star_3PG_ThresholdMap-140.png)
![Noise map NPointGain](images/MGF_star_3PG_NoiseMap-140.png)
![Response curve NPoint Gain](images/MGF_star_3PG_responseCurve.png)

Example of the same plots for a full module are also given:
![Threshold map NPointGain, Full Module](images/MGF_star_fullmodule_3PG_ThresholdMap-140.png)
![Noise map NPointGain, Full Module](images/MGF_star_fullmodule_3PG_NoiseMap-140.png)
![Response curve NPoint Gain, Full Module](images/MGF_star_fullmodule_3PG_responseCurve.png)



### Noise Occupancy
To run a noise occupancy scan execute the following command:
```bash
bin/scanConsole -r configs/controller/itsdaq.json -c configs/connectivity/daqload_setup.json -s configs/scans/star/std_noiseOccCountScan.json -p
```
Also in this case the value of ``ABCs_STR_DEL`` has to be set to the correct value from the Strobe Delay scan.

An example of the occupancy map after a successful noise occupancy scan for a hybrid module is given below.
![Occupancy map Noise Occupancy](images/MGF_star_noiseoccupancy_OccupancyMap-14.png)

An example of the occupancy map after a successful noise occupancy scan for a full module is also given.
![Occupancy map Noise Occupancy, Full Module](images/MGF_star_fullmodule_noiseoccupancy_OccupancyMap-14.png)



### Pixel-like Throshold scan
The traditional approach for Threshold scans in the Strip community is to fix the injected charge to a module and vary the channel threshold, while the Pixel community follows the opposite approach, fixing the thresholds for each of the channela and then perform the scan varying the injected charge.

The pixel-like approach enables to fit a non-reverse S-curve (differently from the strip-like approach) and, by fixing the thresholds in advance, allows to be less susceptible to noise, avoiding potential double-knees/shoulders in the S-curves.

An example of the reverse S-curve and the threshold (``ABCs_BVT``) occupancy map after a successful strip-like threshold scan for a full module is given below.
![S-curve strip-like Th. scan](images/MGF_star_fullmodule_striplikethresholdscan_ABCs_BVT.png)
![Occupancy map strip-like Th. scan](images/MGF_star_fullmodule_striplikethresholdscan_ABCs_BVT_Map.png)

An example of the non-reverse S-curve and the charge (``ABCs_BCAL``) occupancy map after a successful pixel-like threshold scan for a full module is given below.
![S-curve pixel-like Th. scan](images/MGF_star_fullmodule_pixellikethresholdscan_ABCs_BCAL.png)
![Occupancy map pixel-like Th. scan](images/MGF_star_fullmodule_pixellikethresholdscan_ABCs_BCAL_Map.png)


