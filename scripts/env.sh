#!/bin/bash

synth=2500
linth1=2000
linth2=1500
diffth=1500
targettot=7
targetcharge=10000
r="configs/controller/specCfg-rd53a-4x3.json"
c="configs/connectivity/example_rd53a_setup.json"
moduleSN=""

# ./scripts/tune-rd53a-module.sh configs/controller/specCfg-rd53a-4x3.json configs/connectivity/example_rd53a.json ## no localDB
# ./scripts/tune-rd53a-module.sh configs/controller/specCfg-rd53a-4x3.json configs/connectivity/example_rd53a.json -W ## use localDB
if [ "$#" -lt 2 ]; then
    echo "Usage: $0 <controller_config.json> <connectivity_config.json> [<-W>]" >&2
    echo "E.g. $0 $r $c" >&2
    echo "Or $0 $r $c -W" >&2
    exit 1
fi

r=$1
c=$2

echo "Controller config file:" $r
echo "Connectivity config file:" $c

if [ $3 ]
then
    echo "Use localDB: Yes"
else
    echo "Use localDB: No"
fi
