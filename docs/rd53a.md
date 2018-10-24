# RD53A testing with the Single Chip Card
More details about the SCC can be found here: [Single Chip Card](https://twiki.cern.ch/twiki/bin/viewauth/RD53/RD53ATesting#RD53A_Single_Chip_Card_SCC)

## Jumper configuration and power on

![Jumper configuration on the SCC ](images/SCC_JumperConfiguration.jpg)

Default settings for operation in **LDO mode**

- PWR_A and PWR_D: VINA and VIND (LDO operation)
- VDD_PLL_SEL: VDDA (PLL driver from VDDA supply)
- VDD_CML_SEL: VDDA (CML driver from VDDA supply)
- VREF_ADC (internal ADC voltage reference)
- IREF_IO (internal current refetrence)
- IREF_TRIM: Jumper to 3 to set the internal reference current at 4 μA
- Jumpers JP10 and JP11 should be closed in order to use LANE 2 and 3
- Add a PLL_RST jumper
- Add a 5 kΩ resistor for R3 to pull up Vctrl to Vdd (back side of the SCC)

**Make sure that the jumper configuration marked in red is correct before powering the chip!!! Applying too high voltage may kill the chip.**

After all jumpers are placed on the SCC, connect the DisplayPort cable to DP1 and power cable to PWR_IN.

Set the power supply to <span style="color:red">**1.85**</span> V, the current should be around 0.5 A and power on the chip. For the LDO operation, e.g. the jumper configuration shown in previous figure, make sure <span style="color:red"> not to apply higher voltage than **1.85 V**</span>.

# Scan Console for RD53A

More general information about hwo to use the scanConsole, can be found on the main page: [ScanConsole](ScanConsole).

In case you run into problems or have abnormal results please consult the troubleshooting page here: [Troubleshooting](troubleshooting)

## Tuning routine

Examples of the result and how to run different scans is shown below. If you have mastered the basics, you probably just need this information.
Basics tuning routine:

- `std_digitalscan.json` (with `-m 1` to reset masks)
- `std_analogscan.json`
- `diff_tune_globalthreshold.json` (good starting threshold target is 1000e, resets prev. TDACs)
- `diff_tune_pixelthreshold.json` (1000e target again)
- `lin_tune_globalthreshold.json` (good starting threshold target is 2000e, resets prev. TDACs)
- `lin_tune_pixelthreshold.json` (2000e again)
- `lin_retune_globalthreshold.json` (now retuning from 2000e to 1000e target)
- `lin_retune_pixelthreshold.json` (1000e again)
- `syn_tune_globalthreshold.json` (can be as low as 1000e, but keep noise occupancy in check)
- `std_thresholdscan.json` (verify thresholds, use root plot script for nice plots, see [here](rootscripts))
- `std_totscan.json` (with target charge equal to MIP, e.g. 12ke)
- `std_noisescan.json` (measure noise occupancy, will mask noisy pixels, might fail if too noisy)

If you also want to tune the ToT conversion we need to insert those tunings and also some threshold retunings. For a bsic routine including those see below:

- `std_digitalscan.json` (with `-m 1` to reset masks)
- `std_analogscan.json`
- `diff_tune_globalthreshold.json` (good starting threshold target is 1000e, resets prev. TDACs)
- `diff_tune_pixelthreshold.json` (1000e target again)
- `diff_tune_globalpreamp.json` (use mid of the range ToT values, e.g. 10000e at 8ToT)
- `diff_tune_pixelthreshold.json` (1000e target again)
- `lin_tune_globalthreshold.json` (good starting threshold target is 2000e, resets prev. TDACs)
- `lin_tune_pixelthreshold.json` (2000e again)
- `lin_retune_globalthreshold.json` (now retuning from 2000e to 1000e target)
- `lin_retune_pixelthreshold.json` (1000e again)
- `lin_tune_globalpreamp.json` (use mid of the range ToT values, e.g. 10000e at 8ToT)
- `lin_retune_pixelthreshold.json` (1000e again)
- `syn_tune_globalthreshold.json` (can be as low as 1000e, but keep noise occupancy in check)
- `syn_tune_globalpreamp.json` (use mid of the range ToT values, e.g. 10000e at 8ToT)
- `syn_tune_globalthreshold.json` (can be as low as 1000e, but keep noise occupancy in check)
- `std_thresholdscan.json` (verify thresholds, use root plot script for nice plots, see [here](rootscripts))
- `std_totscan.json` (with target charge equal to MIP, e.g. 12ke)
- `std_noisescan.json` (measure noise occupancy, will mask noisy pixels, might fail if too noisy)

## Common misconceptions and issues

Some general tips when operating RD53A with YARR:

- Most scans use a mask loop to scan over pixels, this (currently) ignores the enable mask written to the config. Only data-taking like scans (e.g. noisescan or external trigger scan) will use the enable mask written to the chip config. If for example you run a noise scan and it is completly empty, check the enable mask in your config.
- The target charge command line argument is interpreted in different ways depending on the scan. For a threshold tuning it is used as the threshold target, for a totscan it is used as the value of charge for the injections, and in case of a preamp tuning it is also used the target charge for achieve the specified ToT (second argument).
- There can be interference between the analog FEs, for instance when one FE is badly tuned and very noisy it might radiate noise into other FEs. Hence one should make sure that even when only using one FE type, that the other FEs are in a decent state (e.g. high threshold).
- Auto-zeroing is performed by the hardware and therefore the auto-zero frequency is set by the hardware controller.
- Auto-zeroing can cause transients on the power line which can cause other FEs to be noisy. So if someone chooses not to use the sync FE, one should also turn-off auto-zeroing in the controller config (by setting the auto-zero word to `0`).

## Digital Scan

To run a digital scan for RD53A with the default configuration execute the following command:
```bash
bin/scanConsole -r configs/controller/specCfg.json -c configs/connectivity/example_rd53a_setup.json -s configs/scans/rd53a/std_digitalscan.json -p
```
An example of occupancy map after a successful digital scan is given below.
![Occupancy map digital scan](images/JohnDoe_DigitalScan_OccupancyMap.png)

## Analog Scan

To run a analog scan for RD53A with the default configuration execute the following command:
```bash
bin/scanConsole -r configs/controller/specCfg.json -c configs/connectivity/example_rd53a_setup.json -s configs/scans/rd53a/std_analogscan.json -p
```
An example of the occupancy map after a successful analog scan is given below.
![Occupancy map analog scan](images/JohnDoe_AnalogScan_OccupancyMap.png)

### Analog scan for only one analog FrontEnd

```bash
bin/scanConsole -r configs/controller/specCfg.json -c configs/connectivity/example_rd53a_setup.json -s configs/scans/rd53a/diff_analogscan.json -p
```
![Occupancy map analog scan for DIFF FE](images/JohnDoe_AnalogScanDiff_OccupancyMap.png)
- There are similar scan configs for the linear and sync FE

## Threshold Scan

To run a threshold scan for RD53A with the default configuration execute the following command:
```bash
bin/scanConsole -r configs/controller/specCfg.json -c configs/connectivity/example_rd53a_setup.json -s configs/scans/rd53a/std_thresholdscan.json -p
```
The threshold and noise mean and dispersion value (for everything scanned) will be given in the output of the code, for example:
```text
[0] Threashold Mean = 3245.7 +- 801.668
[0] Noise Mean = 125.784 +- 312.594
```
Example of the s-curve, threshold distribution, threshold map and noise distribution are given below:
![S-curve threshold scan](images/JohnDoe_ThresholdScan_sCurve.png)
![Threshold distribution](images/JohnDoe_ThresholdDist.png)
![Threshold map](images/JohnDoe_ThresholdMap.png)
![Noise distribution](images/JohnDoe_NoiseDist.png)

## Time over Threshold Scan

```bash
bin/scanConsole -r configs/controller/specCfg.json -c configs/connectivity/example_rd53a_setup.json -s configs/scans/rd53a/std_totscan.json -p -t 10000
```
The ToT mean value will be given in the output of the code, for example:
```text
[0] ToT Mean = 8.9869 +- 1.39282
```

ToT scan shown here is after tuning to 8ToT at 10000
![Mean ToT Map](images/JohnDoe_MeanTotMap0.png)
![Mean ToT Distribution](images/JohnDoe_MeanTotDist_0.png)


## Tuning
*To be updated with latest sw changes!*

The tuning usually starts by tuning the global threshold DAC of the FrontEnd you want to tune:

- `DiffVth1` for the differential FE
- `SyncVth` for the synchronous FE
- `LinVth` for the linear FE

Below is an example of tuning the global threshold of the linear FE to 2500e. The goal is too reach 50% occupancy, this usually looks like a symmetric bathtub when the per pixel threshold is untuned.
In a similar manner the differential and synchronous global threshold DAC can be tuned.

```bash
bin/scanConsole -r configs/controller/specCfg.json -c configs/connectivity/example_rd53a_setup.json -s configs/scans/rd53a/lin_tune_globalthreshold.json -t 2500 -p
```
| ![Occupancy distribution](images/JohnDoe_OccupancyDist-0-LinTuning.png) | ![Occupancy distribution](images/JohnDoe_OccupancyDist-1-LinTuning.png) | ![Occupancy distribution](images/JohnDoe_OccupancyDist-3-LinTuning.png) | ![Occupancy distribution](images/JohnDoe_OccupancyDist-4-LinTuning.png) | ![Occupancy distribution](images/JohnDoe_OccupancyDist-5-LinTuning.png) | ![Occupancy distribution](images/JohnDoe_OccupancyDist-6-LinTuning.png) |
|:---:|:---:|:---:|:---:|:---:|:---:|

**Plots to be updated**
Then, the pixel tuning can be performed. See below an example of tuning the differential pixel TDACs to 2500e. The initial bathtub of the global threshold tune appear again, but with each step the values should converge more and more towards the center. In a similar manner the pixel TDACs of the linear FE can be tuned.

```bash
bin/scanConsole -r configs/controller/specCfg.json -c configs/connectivity/example_rd53a_setup.json -s configs/scans/rd53a/diff_tune_pixelthreshold.json -t 2500 -p
```
| ![Occupancy distribution](images/JohnDoe_OccupancyDist-0.png) | ![Occupancy distribution](images/JohnDoe_OccupancyDist-1.png) | ![Occupancy distribution](images/JohnDoe_OccupancyDist-2.png) | ![Occupancy distribution](images/JohnDoe_OccupancyDist-3.png) | ![Occupancy distribution](images/JohnDoe_OccupancyDist-4.png) |
|:---:|:---:|:---:|:---:|:---:|

Once the full tuning routine (as outlined in the beginning of this page) has been performed, you can verify the tuning with a threshold scan:

```text
[0] Threashold Mean = 1050.66 +- 182.54
[0] Noise Mean = 161.395 +- 144.716

```
![Threshold distribution after tuning](images/JohnDoe_ThresholdDist_AfterTuning.png)
![Noise distribution after tuning](images/JohnDoe_NoiseDist_AfterTuning.png)
![S-curve threshold scan after tuning](images/JohnDoe_sCurve_AfterTuning.png)
![Threshold map after tuning](images/JohnDoe_ThresholdMap_AfterTuning.png)


## Cross-talk

The cross-talk is evaluated injecting in the neighboring pixels and checking the occupancy in the central pixel. 

To check if there is cross-talk for your chip+sensor, use the following command:
```bash
bin/scanConsole -r configs/controller/specCfg.json -c configs/connectivity/example_rd53a_setup.json -s configs/scans/rd53a/std_crosstalk.json  -p
```
For the defaul settings and a fully depleated sensor, the OccupancyMap.png plot should show hits in half of the pixels.

## Cross-talk scan

To identify the threhsold at which cross-talk appear, run the following command:
```bash
bin/scanConsole -r configs/controller/specCfg.json -c configs/connectivity/example_rd53a_setup.json -s configs/scans/rd53a/std_crosstalk_scan.json  -p
```

Config parameters:

 - max ``<int>``: number of mask stages
 - min ``<int>``: mask stage to start with
 - step ``<int>``: step size of mask stage (do not use a value lower than 64)
 - maskType  ``<int>``: for standard threshold scans (0), or for cross-talk (1 and 2, depending on the cross-talk definition)
 - maskSize  ``<int>``: define in which neighbouring pixels charge is injected
 - includedPixels ``<int>``: define if and which pixel edges are not considered to measure cross-talk 
 - sensorType  ``<int>``: square sensor (0), rectangular sensor with bump-bond (0,0) bonded with the pixel at the corner (1), and rectangular sensor with bump-bond (0,1) bonded with the pixel at the corner (2)


Example of the s-curve, threshold distribution, threshold map and noise distribution for the tuned linear front-end are given below:
![S-curve threshold scan](images/JohnDoe_crosstalkscan_sCurve.png)
![Threshold distribution](images/JohnDoe_crosstalkscan_ThrehsoldDist.png)


# Loop Actions

List of available loop actions and their configuration parameters.

## Rd53aTriggerLoop
Will repeatably send a 512-bit command from the burst buffer to the FE.

Config parameters:
    
- count ``<int>``: number of injections, if 0 will run for specified time (default 50/100)
- delay ``<int>``: time gap in bunch crossings between injection and trigger, should be divisible by 8 (default 56)
- extTrigger ``<bool>``: enable external triggering, requires proper trigger processors configuration (default false)
- frequency ``<int>``: trigger(/injection)frequency in Hz (default 5000)
- noInject ``<bool>``: disable charge injection (e.g. for noise scan) (default false)
- time ``<int>``: time in seconds, if count is set to 0 will run for this amount of time (default 0)
- edgeMode ``<bool>``: switches cal injection command to edge mode, e.g. for digital scan (default false)

## Rd53aCoreColLoop
Loops of core columns in specified pattern

Config parameters:
    
- max ``<int>``: upper bound of cores to scan
- min ``<int>``: lower bound of cores to scan
- step ``<int>``: step size (1 will scan all cores, more than 1 will skip cores)
- nSteps ``<int>``: how many steps should be used to scan over cores

## Rd53aMaskLoop
Loops over pixels. All pixels in one core column are serialized on the following fashion.
    
```
unsigned serial = (core_row*64)+((col+(core_row%8))%8)*8+row%8;
```

The maximum of the loops defines how many pixels should be activated at one time. E.g. if the max is 64 that means every 64th pixel (1 pixel per core) and requires 64 steps to loop over all pixels.

Config parameters:

 - max ``<int>``: number of mask stages
 - min ``<int>``: mask stage to start with
 - step ``<int>``: step size of mask stage
<<<<<<< HEAD
=======

## Rd53aParameterLoop
Loops over a specifed range of an RD53A global register

Config parameters:

- min ``<int>``: start value (inclusive)
- max ``<int>``: end value (inclusive)
- step ``<int>``: parameter step size
- parameter ``<string>``: name of the global register (as in chip config)

## Rd53aGlobalFeedback
Receives feedback from the selected analysis algorithm and adjusts the selected RD53A global register.

Config parameters:

- min ``<int>``: absolute minimum register setting
- max ``<int>``: start value
- step ``<int>``: stepsize of tuning (might change depending on algorithm)
- parameter ``<string>``: name of the global register (as in chip config) to tune
- pixelRegs ``<0,1,2,3>``: `0` (default) does not change TDACs, `1` sets all TDACs to default, `2` sets all TDACs to max, `3` sets all TDACs to min 

## Rd53aPixelFeedback
Receives feedback from the selected analysis algorithm and adjusts the selected RD53A pixel TDAC register.

Config parameters:

- min ``<int>``: absolute minium setting
- max ``<int>``: absolute max setting
- steps ``<array<int>>``: size and number of tuning steps to be performed
- tuneDiff ``<bool>``: enable adjustment of diff FE pixel regs
- tuneLin ``<bool>``: enable adjustment of lin FE pixel regs
- resetTdac ``<bool>``: reset TDACs to defaults

>>>>>>> upstream/devel
