#!/bin/bash
if [ "$#" -lt 5 ]; then
    echo "Usage: $0 <target_threshold> <target_tot> <target_charge> <controller_config.json> <config_file1.json> [<config_file2.json> ..]" >&2
    exit 1
fi

bin/scanConsole -s digitalscan -r $4 -c ${@:5} -p -m 1
bin/scanConsole -s analogscan -r $4 -c ${@:5} -p
bin/scanConsole -s tune_globalthreshold -t $1 -r $4 -c ${@:5} -p
bin/scanConsole -s tune_globalpreamp -t $3 $2 -r $4 -c ${@:5} -p
bin/scanConsole -s tune_globalthreshold -t $1 -r $4 -c ${@:5} -p
bin/scanConsole -s tune_pixelthreshold -t $1 -r $4 -c ${@:5} -p
bin/scanConsole -s tune_globalpreamp -t $3 $2 -r $4 -c ${@:5} -p
bin/scanConsole -s tune_pixelpreamp -t $3 $2 -r $4 -c ${@:5} -p
bin/scanConsole -s tune_pixelthreshold -t $1 -r $4 -c ${@:5} -p
bin/scanConsole -s noisescan -r $4 -c ${@:5} -p
bin/scanConsole -s totscan -t $3 $2 -r $4 -c ${@:5} -p
bin/scanConsole -s thresholdscan -r $4 -c ${@:5} -p
