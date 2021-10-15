#!/bin/bash

if [ "$#" -lt 3 ]; then
    echo "Usage: $0 <controller config> <chip config> <output dir>" >&2
    exit 1
fi

controllerCfg=$1
shift
inputChipCfg=$1
shift
outputDir=$1

mkdir -vp $outputDir/configs

for diffPreamp in 400 900; do
    for diffVff in 60 150; do
        for diffLccEn in 0 1; do
            for tdac in -15 0 15; do
                for threshold in 1000 2000; do
                    outputChipCfg=${outputDir}/configs/chipCfg_diffPreamp${diffPreamp}_diffVff${diffVff}_diffLccEn${diffLccEn}_TDAC${tdac}_${threshold}e.json
                    python scripts/modifyChipConfig.py -i $inputChipCfg -o $outputChipCfg --diffPreamp ${diffPreamp} --diffVff ${diffVff} --diffLccEn ${diffLccEn} --tdac ${tdac} --diffPreComp 300 --diffComp 500 --th2 0
                    echo "Chip config ${outputChipCfg} created"

                    # This script assumes Rx = 0 and Tx = 0
                    # If not, please modify the connectivity file below
                    outputConnCfg=${outputDir}/configs/connCfg_diffPreamp${diffPreamp}_diffVff${diffVff}_diffLccEn${diffLccEn}_TDAC${tdac}_${threshold}e.json
cat > ${outputConnCfg} <<EOF
{
    "chipType" : "RD53B",
    "chips" : [
        {
            "config" : "${outputChipCfg}",
            "tx" : 0,
            "rx" : 0,
            "enable" : 1,
            "locked" : 0
        }
    ]
}
EOF
                    echo "Connectivity config ${outputConnCfg} created"

                    outputDataDir=$outputDir/data_diffPreamp${diffPreamp}_diffVff${diffVff}_diffLccEn${diffLccEn}_TDAC${tdac}_${threshold}e
                    ./bin/scanConsole -r ${controllerCfg} -c ${outputConnCfg} -s configs/scans/rd53b/std_digitalscan.json -n 1 -p -m 1 -o ${outputDataDir}
                    ./bin/scanConsole -r ${controllerCfg} -c ${outputConnCfg} -s configs/scans/rd53b/ptot_retune_globalthreshold_${threshold}e.json -n 1 -t ${threshold} -p -o ${outputDataDir}
                    ./bin/scanConsole -r ${controllerCfg} -c ${outputConnCfg} -s configs/scans/rd53b/ptot_thresholdscan_${threshold}e.json -n 1 -p -o ${outputDataDir}
                done
            done
        done
    done
done

