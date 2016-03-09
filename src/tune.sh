#!/bin/bash
bin/scanConsole -s digitalscan -c scanConsole_example.gcfg -p -o plots
bin/scanConsole -s analogscan -c scanConsole_example.gcfg -p -o plots
bin/scanConsole -s tune_globalthreshold -c scanConsole_example.gcfg -p -o plots
bin/scanConsole -s tune_globalpreamp -c scanConsole_example.gcfg -p -o plots
bin/scanConsole -s tune_globalthreshold -c scanConsole_example.gcfg -p -o plots
bin/scanConsole -s tune_pixelthreshold -c scanConsole_example.gcfg -p -o plots
bin/scanConsole -s tune_globalpreamp -c scanConsole_example.gcfg -p -o plots
bin/scanConsole -s tune_pixelpreamp -c scanConsole_example.gcfg -p -o plots
bin/scanConsole -s tune_pixelthreshold -c scanConsole_example.gcfg -p -o plots
bin/scanConsole -s noisescan -c scanConsole_example.gcfg -p -o plots
bin/scanConsole -s totscan -c scanConsole_example.gcfg -p -o plots
bin/scanConsole -s thresholdscan -c scanConsole_example.gcfg -p -o plots
