#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
source $SCRIPT_DIR/env.sh

# Final threshold scan with forward-biased sensor for checking disconnected bumps
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/syn_thresholdscan.json -p -n 1 $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/lin_thresholdscan.json -p -n 1 $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/diff_thresholdscan.json -p -n 1 $3
