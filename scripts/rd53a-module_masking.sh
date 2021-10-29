#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
source $SCRIPT_DIR/env.sh

# Create final mask for operation
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/std_digitalscan.json -p -m 1 -n 1 $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/syn_analogscan.json -p -n 1 $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/lindiff_analogscan.json -p -n 1 $3
## repeat noise scan several times if necessary
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/syn_noisescan.json -p -n 1 $3
./bin/scanConsole -r $r -c $c -s configs/scans/rd53a/lindiff_noisescan.json -p -n 1 $3
