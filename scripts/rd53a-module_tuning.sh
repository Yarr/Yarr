#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
source $SCRIPT_DIR/env.sh


# Untuned scans
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/std_digitalscan.json -p -m 1 $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/syn_analogscan.json -p $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/lindiff_analogscan.json -p $3

./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/syn_thresholdscan.json -p $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/lin_thresholdscan.json -p $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/diff_thresholdscan.json -p $3

./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/syn_totscan.json -p -t $targetcharge  $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/lin_totscan.json -p -t $targetcharge  $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/diff_totscan.json -p -t $targetcharge  $3


# Tune sync FE
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/syn_tune_globalthreshold.json -t $synth $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/syn_tune_globalpreamp.json -t $targetcharge $targettot $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/syn_tune_globalthreshold.json -t $synth -p $3


# Tune lin FE
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/lin_tune_globalthreshold.json -t $linth1 $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/lin_tune_pixelthreshold.json -t $linth1 $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/lin_retune_globalthreshold.json -t $linth2 $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/lin_retune_pixelthreshold.json -t $linth2 $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/lin_tune_globalpreamp.json -t $targetcharge $targettot -p $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/lin_retune_pixelthreshold.json -t $linth2 -p $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/lin_tune_finepixelthreshold.json -t $linth2 -p $3


# Tune diff FE
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/diff_tune_globalthreshold.json -t $diffth $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/diff_tune_globalpreamp.json -t $targetcharge $targettot $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/diff_tune_globalthreshold.json -t $diffth -p $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/diff_tune_pixelthreshold.json -t $diffth -p $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/diff_tune_finepixelthreshold.json -t $diffth -p $3


# Final threshold & tot scans
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/syn_thresholdscan.json -p $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/lin_thresholdscan.json -p $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/diff_thresholdscan.json -p $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/syn_totscan.json -p -t $targetcharge $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/lin_totscan.json -p -t $targetcharge $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/diff_totscan.json -p -t $targetcharge $3


# Crosstalk and disconnected bump scan
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/syn_crosstalkscan.json -p $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/syn_discbumpscan.json -p $3

./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/lin_crosstalkscan.json -p $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/lin_discbumpscan.json -p $3

./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/diff_crosstalkscan.json -p $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/diff_discbumpscan.json -p $3
