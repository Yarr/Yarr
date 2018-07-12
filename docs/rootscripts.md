#Root Scripts

Scripts currently just for RD53A. 

Requires root6.

##Compile with Makefile

```bash
$ cd Yarr/src/scripts
$ make
[Compiling] plotWithRoot_ThresholdTDAC.o
[Linking] plotWithRoot_ThresholdTDAC
[Compiling] plotWithRoot_TDAC.o
[Linking] plotWithRoot_TDAC
[Compiling] plotWithRoot_Threshold.o
[Linking] plotWithRoot_Threshold
[Compiling] plotWithRoot_Occupancy.o
[Linking] plotWithRoot_Occupancy
[Compiling] plotWithRoot_Noise.o
[Linking] plotWithRoot_Noise
```

##Scripts

###Occupancy Plots

Plots 1D histograms showing [Number of Pixels] versus [Range of Occupancy Values (%)] for each of the RD53A front ends (FEs) as well as a stacked histogram.
Uses *OccupancyMap.dat files.
Currently the script assumes the targeted occupancy value is 100 injections.

```bash
$ ./plotWithRoot_Occupancy path/to/directory
```

Examples of all the Occupancy plots given below: 

![Occupancy Plots Preview](/images/OccupancyPlots_Preview.png)

###Threshold Plots

Plots and fits 1D histograms showing [Number of Pixels] versus [Threshold Value (e)] for each of the RD53A FEs and one stacked plot; the combined plot is used to get the fit for the stacked plot.
Plots 1D histograms showing [Number of Pixels] versus [Range of Threshold Values (deviation from the mean)] for each of the RD53A FEs.
Plots 2D histogram showing the [Threshold Value (e)] for each pixel.


```bash
$ ./plotWithRoot_Threshold path/to/directory
```

Examples of some of the Threshold plots given below: 

![Threshold Plots Preview](/images/ThresholdPlots_Preview.png)
![Threshold2D Plot Preview](/images/Threshold2DPlot_Preview.png)


###Noise Plots

Plots and fits 1D histograms showing [Number of Pixels] versus [Noise (e)] for each of the RD53A FEs and one stacked plot.
Plots 1D histograms showing [Number of Pixels] versus [Range of Noise (deviation from the mean)] for each of the RD53A FEs.
Plots 2D histogram showing the [Noise (e)] for each pixel.

```bash
$ ./plotWithRoot_Noise path/to/directory
```

Examples of some of the Noise plots given below: 

![Noise Plots Preview](/images/NoisePlots_Preview.png)
![Noise2D Plot Preview](/images/Noise2DPlot_Preview.png)

###TDAC Plots

Plots and fits 1D histograms showing [Number of Pixels] versus [TDAC setting] for the linear and differential RD53A FEs and one stacked plot.
Plots 1D histograms showing [Number of Pixels] versus [Range of TDAC settings (deviation from the mean)] for each of the RD53A FEs.
Plots 2D histogram showing the [TDAC setting] for each pixel.

```bash
$ ./plotWithRoot_TDAC path/to/directory
```

Examples of some of the TDAC plots given below: 

![TDAC Plots Preview](/images/TDACPlots_Preview.png)

###Threshold/TDAC plots

Plots a stacked histogram for all thresholds and their corresponding TDACs.
Uses .json.after and ThresholdMap.dat files; this program is currently uses the first of each file found.

```bash
$ ./plotWithRoot_ThresholdTDAC path/to/directory
```

Examples of some of the ThresholdTDAC plots given below: 

![ThresholdTDAC Plots Preview](/images/ThresholdTDACPlots_Preview.png)



