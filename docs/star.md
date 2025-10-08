# `libStar`

`libStar` provides FrontEnd classes, loop actions, analyses, and standalone applications for use with ITk Strips ASICs.

## Applications

`libStar` provides a few standalone applications.
Available programs and some basic information about each can be seen below.

### `test_star`

`test_star` is a program that provides various test suites for front-end communication.

The primary argument is for the controller configuration:

```bash
bin/test_star configs/controller/felix_client_strips.json
```

In order to obtain useful results, you will need to provide valid rx channels:

```bash
bin/test_star -r 0 2 4 configs/controller/emuCfg_star.json
```

By default, the full suite of tests will be run. Parts of tests can also be run individually:

```bash
bin/test_star -r 0 2 4 -s checkHPRs configs/controller/emuCfg_star.json
```

For more information (and a list of available tests) see the help message:

```bash
bin/test_star --help
```

### `printStarSubRegisters`

Prints out all registers and subregisters in human-readable format from a chip config file.
Especially useful to examine post-scan configs for specific subregister values.

### `makeLCBSequence`

Creates an LCB sequence from a list of commands.

### `lcbL0a`

Sends a fully-configurable L0A and/or BCR.
Requires a hardware controller configuration file for front-end communication.

A basic L0A (mask `0b0001`, tag `0x00`, no BCR) can be sent on Tx channel 1 with:

```
./bin/lcbL0a controller_config.json --tag 0 --mask 1
```

To send a more complex L0A (mask `0b1101`, tag `0x18`, BCR) on Tx channel 6, use e.g.

```
./bin/lcbL0a controller_config.json --tx 6 --tag 0x18 --mask 1101 --bcr
```

The L0A tag can be provided in hex (with the prefix `0x`) or decimal.

### `lcbFastCommand`

Sends a user-specified fast command.
Requires a hardware controller configuration file for front-end communication.

Fast command types can either be specified by a string identifier or a numerical identifier, e.g.

```
./bin/lcbFastCommand controller_config.json logic-reset
```

and

```
./bin/lcbFastCommand controller_config.json 2
```

will produce the same results.
A list of all available fast commands (and their numerical codes) can be seen by running

```
./bin/lcbFastCommand --show-fast-commands
```

Fast command delays can be set with the `--delay` argument.


### `lcbRegisterCommand`

Sends a user-specified HCC/ABC register read/write command.
Requires a hardware controller configuration file for front-end communication.

The general format of the command is

```
./bin/lcbRegisterCommand <controller-config> <read|write> <hcc|abc> <other-options>
```

where `read` or `write` are used to specify the action to use, and `hcc` or `abc` are used to specify the chip type.

For example, to send a register read for register 40 to an ABC with chip ID 2 through and HCC on Tx channel `0x46`, Rx channel `0x40` with HCC ID 0:

```
./bin/lcbRegisterCommand controller_config.json read abc --tx 0x46 --rx 0x40 --hcc-id 0 --abc-id 2 --address 40
```

To broadcast across ABCs or HCCs, set `--abc-id`/`--hcc-id` to 15 (or omit it, as the default for each is 15).

Note that if the `write` action is specified, you must also specify a value, e.g.

```
./bin/lcbRegisterCommand controller_config.json write hcc --hcc-id 10 --address 40 --value 0xdeadbeef
```

will write register 40 on HCC with ID 10 on Tx channel 1 with the value `0xdeadbeef`.
Register values (and all other arguments) can be supplied in hex (with the prefix `0x`) or decimal.

Register read commands will read all data received within the specified timeout period (`--timeout`, default 1 second), which allows for all packets to be processed from broadcasted reads.
The data collection can also be disabled entirely with `--send-only`.

## Loop Actions

`libStar` provides a set of strips-specific loop actions.
Available loop actions and their configuration parameters can be seen below.

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
- fpath_sequence ``<string>``: path to the file containing a pre-computed sequence of bytes for setting trigger words

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
![Occupancy map Noise Occupancy, Full Module](images/MGF_star_fullmodule_noiseoccupancy_OccupancyMap-25.png)



### Pixel-like Throshold scan
The traditional approach for Threshold scans in the Strip community is to fix the injected charge to a module and vary the channel threshold, while the Pixel community follows the opposite approach, fixing the thresholds for each of the channela and then perform the scan varying the injected charge.

The pixel-like approach enables to fit a non-reverse S-curve (differently from the strip-like approach) and, by fixing the thresholds in advance, allows to be less susceptible to noise, avoiding potential double-knees/shoulders in the S-curves.

An example of the reverse S-curve and the threshold (``ABCs_BVT``) occupancy map after a successful strip-like threshold scan for a full module is given below.
![S-curve strip-like Th. scan](images/MGF_star_fullmodule_striplikethresholdscan_ABCs_BVT.png)
![Occupancy map strip-like Th. scan](images/MGF_star_fullmodule_striplikethresholdscan_ABCs_BVT_Map.png)

An example of the non-reverse S-curve and the charge (``ABCs_BCAL``) occupancy map after a successful pixel-like threshold scan for a full module is given below.
![S-curve pixel-like Th. scan](images/MGF_star_fullmodule_pixellikethresholdscan_ABCs_BCAL.png)
![Occupancy map pixel-like Th. scan](images/MGF_star_fullmodule_pixellikethresholdscan_ABCs_BCAL_Map.png)

## Printing Subregister Values from Configuration Files

By default, YARR outputs chip configs in terms of only register values.
For easier human readability, `printStarSubRegisters` can convert a chip config into subregister values.
`printStarSubRegisters` is a program that, given a chip config (and ABC/HCC versions) as input, will print out the values of all ABC/HCC registers and subregisters.

```
./bin/printStarSubRegisters -c [CHIP_CONFIG] -r [HCC_VERSION] -a [ABC_VERSION]
```
