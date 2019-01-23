#!/bin/bash


./bin/scanConsole -r $1 -c $2 -s configs/scans/rd53a/std_digitalscan.json -p -m 1 -o $3
./bin/scanConsole -r $1 -c $2 -s configs/scans/rd53a/diff_analogscan.json -p -o $3
./bin/scanConsole -r $1 -c $2 -s configs/scans/rd53a/diff_intimeanalogscan.json -p -m 0 -o $3

./bin/scanConsole -r $1 -c $2 -s configs/scans/rd53a/diff_tune_globalthreshold.json -t $4 -p -m 0 -o $3
./bin/scanConsole -r $1 -c $2 -s configs/scans/rd53a/diff_tune_pixelthreshold.json -t $4 -p -m 0 -o $3
./bin/scanConsole -r $1 -c $2 -s configs/scans/rd53a/diff_tune_globalpreamp.json -t 7500 6 -p -m 0 -o $3
./bin/scanConsole -r $1 -c $2 -s configs/scans/rd53a/diff_retune_pixelthreshold.json -t $4 -p -m 0 -o $3
./bin/scanConsole -r $1 -c $2 -s configs/scans/rd53a/diff_tune_finepixelthreshold.json -p -m 0 -o $3

./bin/scanConsole -r $1 -c $2 -s configs/scans/rd53a/diff_thresholdscan.json -p -m 0 -o $3
scripts/plotWithRoot_Threshold ${3}/last_scan pdf 1
scripts/plotWithRoot_ThresholdTDAC ${3}/last_scan pdf 1
scripts/plotWithRoot_NoiseMap ${3}/last_scan pdf 1
./bin/scanConsole -r $1 -c $2 -s configs/scans/rd53a/diff_intimethresholdscan.json -p -m 0 -o $3
scripts/plotWithRoot_Threshold ${3}/last_scan pdf 1
scripts/plotWithRoot_ThresholdTDAC ${3}/last_scan pdf 1
scripts/plotWithRoot_NoiseMap ${3}/last_scan pdf 1
./bin/scanConsole -r $1 -c $2 -s configs/scans/rd53a/std_totscan.json -p -m 0 -t 6000 -o $3
scripts/plotWithRoot_ToT ${3}/last_scan pdf 1
./bin/scanConsole -r $1 -c $2 -s configs/scans/rd53a/diff_noisescan.json -p -o $3
