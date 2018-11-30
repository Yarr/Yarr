#!/bin/bash


./bin/scanConsole -r $1 -c $2 -s configs/scans/rd53a/std_digitalscan.json -p -m 1 -o $3
./bin/scanConsole -r $1 -c $2 -s configs/scans/rd53a/syn_analogscan.json -p -m 0 -o $3
./bin/scanConsole -r $1 -c $2 -s configs/scans/rd53a/syn_intimeanalogscan.json -p -m 0 -o $3

./bin/scanConsole -r $1 -c $2 -s configs/scans/rd53a/syn_tune_globalthreshold.json -t $4 -p -m 0 -o $3
./bin/scanConsole -r $1 -c $2 -s configs/scans/rd53a/syn_tune_globalpreamp.json -t 5600 6 -p -m 0 -o $3
./bin/scanConsole -r $1 -c $2 -s configs/scans/rd53a/syn_retune_pixelthreshold.json -t $4 -p -m 0 -o $3

./bin/scanConsole -r $1 -c $2 -s configs/scans/rd53a/syn_thresholdscan.json -p -m 0 -o $3
scripts/plotWithRoot_Threshold data/last_scan pdf 1
scripts/plotWithRoot_ThresholdTDAC data/last_scan pdf 1
./bin/scanConsole -r $1 -c $2 -s configs/scans/rd53a/syn_intimethresholdscan.json -p -m 0 -o $3
scripts/plotWithRoot_Threshold data/last_scan pdf 1
scripts/plotWithRoot_ThresholdTDAC data/last_scan pdf 1
./bin/scanConsole -r $1 -c $2 -s configs/scans/rd53a/std_totscan.json -p -m 0 -t 6000 -o $3
./bin/scanConsole -r $1 -c $2 -s configs/scans/rd53a/std_noisescan.json -p -o $3