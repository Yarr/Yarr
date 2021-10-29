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


# Crosstalk and disconnected bump scan
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/syn_crosstalkscan.json -p $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/syn_discbumpscan.json -p $3

./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/lin_crosstalkscan.json -p $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/lin_discbumpscan.json -p $3

./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/diff_crosstalkscan.json -p $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/diff_discbumpscan.json -p $3
