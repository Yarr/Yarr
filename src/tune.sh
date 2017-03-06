#!/bin/bash
if [ "$#" -lt 5 ]; then
    echo "Usage: $0 <target_threshold> <target_tot> <target_charge> <controller_config.json> <config_file1.json> [<config_file2.json> ..]" >&2
    exit 1
fi

bin/scanConsole -s digitalscan -t $1 $2 $3 -r $4 -c ${@:5} -p
bin/scanConsole -s analogscan -t $1 $2 $3 -r $4 -c ${@:5} -p
bin/scanConsole -s tune_globalthreshold -t $1 $2 $3 -r $4 -c ${@:5} -p
bin/scanConsole -s tune_globalpreamp -t $1 $2 $3 -r $4 -c ${@:5} -p
bin/scanConsole -s tune_globalthreshold -t $1 $2 $3 -r $4 -c ${@:5} -p
bin/scanConsole -s tune_pixelthreshold -t $1 $2 $3 -r $4 -c ${@:5} -p
bin/scanConsole -s tune_globalpreamp -t $1 $2 $3 -r $4 -c ${@:5} -p
bin/scanConsole -s tune_pixelpreamp -t $1 $2 $3 -r $4 -c ${@:5} -p
bin/scanConsole -s tune_pixelthreshold -t $1 $2 $3 -r $4 -c ${@:5} -p
bin/scanConsole -s noisescan -t $1 $2 $3 -r $4 -c ${@:5} -p
bin/scanConsole -s totscan -t $1 $2 $3 -r $4 -c ${@:5} -p
bin/scanConsole -s thresholdscan -t $1 $2 $3 -r $4 -c ${@:5} -p
