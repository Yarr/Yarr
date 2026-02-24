# Scan Analysis Algorithms

Within the scan structure, an analysis provides information which can
be derived from more than one bin of the scan.

They works by processing data provided by the histogrammers, and therefore
most algorithms assume the presence of a particular histogrammer in the
configuration.

In addition, some analyses provide feedback to the scan engine on what to do
next. For instance by reporting that the current threshold value is too high
or too low.

## DelayAnalysis

Analyse the delay reported in [L13D](scan_histogrammers.md#l13d) histograms.

## HistogramArchiver

Save histograms to disk. The directory is specified on the command line.

## L1Analysis

Summarise output from the  [L1Dist](scan_histogrammers.md#l1dist) histograms.

## NPointGain

Process threshold and noise histograms, for strips analysis. Takes the
threshold for a median occupancy for a sequence of scans and fits the
response curve.

## NoiseAnalysis

Summarise noise histograms.

## NoiseTuning

Process noise histograms providing feedback based on hit count.

## OccGlobalThresholdTune

Tune global threshold based on occupancy.

## OccPixelThresholdTune

Feedback per Pixel tuning based on occupancy.

## OccupancyAnalysis

Analyse occupancy and build map of pixels to mask.

## ParameterAnalysis

Produce generic summary of occupancy histograms.

## ScurveFitter

Run scurve fit on sequence of occupancy histograms.

## TagAnalysis

Process tag histograms.

## TotAnalysis

Process Tot histograms.

## TODO fill in more information

Each analysis could specify the following:

* Configuration
* Inputs
* Outputs
* Feedback
