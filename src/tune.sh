#!/bin/bash
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <config_file1.json> [<config_file2.json> ..]" >&2
    exit 1
fi
bin/scanConsole -s digitalscan -c $@ -p
bin/scanConsole -s analogscan -c $@ -p
bin/scanConsole -s tune_globalthreshold -c $@ -p
bin/scanConsole -s tune_globalpreamp -c $@ -p
bin/scanConsole -s tune_globalthreshold -c $@ -p
bin/scanConsole -s tune_pixelthreshold -c $@ -p
bin/scanConsole -s tune_globalpreamp -c $@ -p
bin/scanConsole -s tune_pixelpreamp -c $@ -p
bin/scanConsole -s tune_pixelthreshold -c $@ -p
bin/scanConsole -s noisescan -c $@ -p
bin/scanConsole -s totscan -c $@ -p
bin/scanConsole -s thresholdscan -c $@ -p
