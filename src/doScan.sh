#!/bin/bash
#################################
# Contacts: Eunchong Kim (kim@hep.phys.titech.ac.jp)
# Contacts: Arisa Kubota (akubota@hep.phys.titech.ac.jp)
# Project: Yarr
# Description: Run scan
# Usage: ./doScan.sh [-a <rd53a/fei4b>] [-m <SerialNumber>] [-s <ScanType>] [-i <UserConfig>] [-c <TargetCharge>] [-t <TargetTot>] [-r <Controller>] [-d]
# Date: Feb 2019
################################

function usage {
    cat <<EOF

Usage:
    ./$(basename ${0}) [-a <rd53a/fei4b>] [-m <SerialNumber>] [-s <ScanType>] [-i <UserConfig>] [-c <TargetCharge>] [-t <TargetTot>] [-r <Controller>] [-d]

EOF
}

run_info=configs/testRunInfo.json
controller=configs/controller/specCfg.json
database=false

while getopts a:m:s:r:i:c:t:d OPT
do
    case ${OPT} in
        a ) asic_type=${OPTARG} ;;
        m ) mod_id=${OPTARG} ;;
        s ) scan_type=${OPTARG} ;;
        i ) run_info=${OPTARG} ;;
        c ) target_charge=${OPTARG} ;;
        t ) target_tot=${OPTARG} ;;
        r ) controller=${OPTARG} ;;
        d ) database=true ;;
        * ) usage
            exit ;;
    esac
done

if [ -z ${asic_type} ]; then
    echo "Please give \"rd53a\" or \"fei4b\" with '-a'."
    usage
    exit
elif [ ${asic_type} == "rd53a" ]; then
    chiptype="RD53A"
elif [ ${asic_type} == "fei4b" ]; then
    chiptype="FEI4B"
else
    echo "Please give \"rd53a\" or \"fei4b\" with '-a'."
    usage
    exit
fi

if [ -z ${mod_id} ]; then
    echo "Please give serial number with '-m'."
    usage
    exit
fi

if [ -z ${scan_type} ]; then
    echo "Please give scan type with '-s'."
    usage
    exit
fi

if [ ! -f ${run_info} ]; then
    echo "Not exist user config file \"${run_info}\"."
    usage
    exit
fi

if [ ! -f ${controller} ]; then
    echo "Not exist controller \"${controller}\"."
    usage
    exit
fi

echo ${target_charge} | grep [^0-9] > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "Please give an integral as target charge with '-c'. "
    usage
    exit
fi

echo ${target_tot} | grep [^0-9] > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "Please give an integral as target tot with '-t'. "
    usage
    exit
fi

if [ ! -e configs/${asic_type}/${mod_id}/connectivity.json ]; then
    echo "File not found : configs/${asic_type}/${mod_id}/connectivity.json"
    exit
fi

if ${database}; then
    ./bin/scanConsole -r ${controller} -c configs/${asic_type}/${mod_id}/connectivity.json -p -W -I ${run_info} -t ${target_charge} ${target_tot} -s ${scan_type}
else
    ./bin/scanConsole -r ${controller} -c configs/${asic_type}/${mod_id}/connectivity.json -p -t ${target_charge} ${target_tot} -s ${scan_type}
fi
