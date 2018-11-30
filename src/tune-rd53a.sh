#!/bin/bash

if [ "$#" -lt 6 ]; then
    echo "Usage: $0 <first_target_threshold> <second_target_threshold> <target_charge> <target_tot> <controller_config.json> <config_file.json> [<config_file2.json> ..] " >&2
    exit 1
fi

./bin/scanConsole -r $5 -c ${@:6} -s configs/scans/rd53a/std_digitalscan.json -p -m 1
./bin/scanConsole -r $5 -c ${@:6} -s configs/scans/rd53a/std_analogscan.json -p
#./bin/scanConsole -r $5 -c ${@:6} -s configs/scans/rd53a/std_thresholdscan.json -p -m
#./bin/scanConsole -r $5 -c ${@:6} -s configs/scans/rd53a/std_totscan.json -p -m 0 -t $3

./bin/scanConsole -r $5 -c ${@:6} -s configs/scans/rd53a/diff_tune_globalthreshold.json -t $2 -p -m 0
./bin/scanConsole -r $5 -c ${@:6} -s configs/scans/rd53a/diff_tune_pixelthreshold.json -t $2 -p -m 0
./bin/scanConsole -r $5 -c ${@:6} -s configs/scans/rd53a/diff_tune_globalpreamp.json -t $3 $4 -p -m 0
./bin/scanConsole -r $5 -c ${@:6} -s configs/scans/rd53a/diff_tune_pixelthreshold.json -t $2 -p -m 0

./bin/scanConsole -r $5 -c ${@:6} -s configs/scans/rd53a/lin_tune_globalthreshold.json -t $1 -p -m 0
./bin/scanConsole -r $5 -c ${@:6} -s configs/scans/rd53a/lin_tune_pixelthreshold.json -t $1 -p -m 0
./bin/scanConsole -r $5 -c ${@:6} -s configs/scans/rd53a/lin_retune_globalthreshold.json -t $2 -p -m 0
./bin/scanConsole -r $5 -c ${@:6} -s configs/scans/rd53a/lin_retune_pixelthreshold.json -t $2 -p -m 0
./bin/scanConsole -r $5 -c ${@:6} -s configs/scans/rd53a/lin_tune_globalpreamp.json -t $3 $4 -p -m 0
./bin/scanConsole -r $5 -c ${@:6} -s configs/scans/rd53a/lin_retune_pixelthreshold.json -t $2 -p -m 0

./bin/scanConsole -r $5 -c ${@:6} -s configs/scans/rd53a/syn_tune_globalthreshold.json -t $2 -p -m 0
./bin/scanConsole -r $5 -c ${@:6} -s configs/scans/rd53a/syn_tune_globalpreamp.json -t $3 $4 -p -m 0
./bin/scanConsole -r $5 -c ${@:6} -s configs/scans/rd53a/syn_tune_globalthreshold.json -t $2 -p -m 0

./bin/scanConsole -r $5 -c ${@:6} -s configs/scans/rd53a/std_thresholdscan.json -p -m 0
./bin/scanConsole -r $5 -c ${@:6} -s configs/scans/rd53a/std_totscan.json -p -m 0 -t $3
./bin/scanConsole -r $5 -c ${@:6} -s configs/scans/rd53a/std_noisescan.json -p
