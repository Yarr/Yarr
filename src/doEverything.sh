#!/bin/bash
#################################
# Contacts: Eunchong Kim (kim@hep.phys.titech.ac.jp)
# Contacts: Arisa Kubota (akubota@hep.phys.titech.ac.jp)
# Project: Yarr
# Description: Run QA scan
# Usage: ./doEverything.sh [-a <rd53a/fei4b>] [-m <SerialNumber>] [-c <ChipNum>] [-i <UserConfig>] [-r <Controller>] [-d]
# Date: Feb 2019
################################

function usage {
    cat <<EOF

Usage:
    ./$(basename ${0}) [-a <rd53a/fei4b>] [-m <SerialNumber>] [-c <ChipNum>] [-i <UserConfig>] [-r <Controller>] [-d]

EOF
}

run_info=configs/testRunInfo.json
controller=configs/controller/specCfg.json
database=""

while getopts a:m:r:i:c:d OPT
do
    case ${OPT} in
        a ) asic_type=${OPTARG} ;;
        m ) mod_id=${OPTARG} ;;
        c ) chips=${OPTARG} ;;
        i ) run_info=${OPTARG} ;;
        r ) controller=${OPTARG} ;;
        d ) database="-d" ;;
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

echo ${chips} | grep [^0-9] > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "Please give an integral as number of chips with '-c'. "
    usage
    exit
fi

if [ ${database} == "-d" ]; then
    cd configs
    ./create_${asic_type}_config.sh -a ${asic_type} -m ${mod_id} -i ${run_info#*/} -c ${chips} -r ${controller#*/}
    cd ../
fi

./doScan.sh -a ${asic_type} -m ${mod_id} -i ${run_info} -r ${controller} ${database} -s digitalscan

./doScan.sh -a ${asic_type} -m ${mod_id} -i ${run_info} -r ${controller} ${database} -s thresholdscan

./doScan.sh -a ${asic_type} -m ${mod_id} -i ${run_info} -r ${controller} ${database} -s tune_globalthreshold -c 2668

./doScan.sh -a ${asic_type} -m ${mod_id} -i ${run_info} -r ${controller} ${database} -s thresholdscan

./doScan.sh -a ${asic_type} -m ${mod_id} -i ${run_info} -r ${controller} ${database} -s tune_pixelthreshold -c 2668

./doScan.sh -a ${asic_type} -m ${mod_id} -i ${run_info} -r ${controller} ${database} -s thresholdscan

./doScan.sh -a ${asic_type} -m ${mod_id} -i ${run_info} -r ${controller} ${database} -s tune_globalpreamp -c 14941 -t 9

./doScan.sh -a ${asic_type} -m ${mod_id} -i ${run_info} -r ${controller} ${database} -s totscan

./doScan.sh -a ${asic_type} -m ${mod_id} -i ${run_info} -r ${controller} ${database} -s tune_pixelpreamp -c 14941 -t 9

./doScan.sh -a ${asic_type} -m ${mod_id} -i ${run_info} -r ${controller} ${database} -s totscan

./doScan.sh -a ${asic_type} -m ${mod_id} -i ${run_info} -r ${controller} ${database} -s digitalscan

./doScan.sh -a ${asic_type} -m ${mod_id} -i ${run_info} -r ${controller} ${database} -s analogscan

./doScan.sh -a ${asic_type} -m ${mod_id} -i ${run_info} -r ${controller} ${database} -s thresholdscan

./doScan.sh -a ${asic_type} -m ${mod_id} -i ${run_info} -r ${controller} ${database} -s noisescan

./doScan.sh -a ${asic_type} -m ${mod_id} -i ${run_info} -r ${controller} ${database} -s selftrigger
