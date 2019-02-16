#!/bin/bash
#################################
# Contacts: Eunchong Kim (kim@hep.phys.titech.ac.jp)
# Contacts: Arisa Kubota (akubota@hep.phys.titech.ac.jp)
# Contacts: Shohei Yamagaya (yamagaya@champ.hep.sci.osaka-u.ac.jp) 
# Project: Yarr
# Description: Run scan
# Usage: ./doScan_fei4b.sh [-m <SerialNumber>*] [-s <ScanType>*] [-r <ControllerCfg>] [-n <ConnecitiityCfg>] [-i <EnvironmentalCfg>] [-c <TargetCharge>] [-t <TargetTot>] [-p <TargetPreamp>] [-M <mask>] [-d] [-l]
# * is required item
# Date: Feb 2019
################################

# default parameters
asic_type=fei4b
target_tot=9
target_preamp=14941
target_charge=2668
mask=-1
database=false
getlog=false

function usage {
    cat <<EOF

Usage:
    ./$(basename ${0}) [-m SN*] [-s Scan*] [-r Controller] [-n Connectivity] [-i Environment] [-c Charge] [-t Tot] [-p Preamp] [-M mask] [-d] [-l]

Options:
    -m <str>      serial number (*req.)
    -s <str/path> scan type (*req.)
    -r <path>     controller config file            default: ./configs/${asic_type}/\${mod_id}/controller.json
    -n <path>     connectivity config file          default: ./configs/${asic_type}/\${mod_id}/connectivity.json
    -i <path>     environmental config file         default: ./configs/${asic_type}/\${mod_id}/info.json
    -c <int>      target charge for tune_threshold  default: 2668
    -t <int>      target tot for tune_preamp        default: 9
    -p <int>      target preamp for tune_preamp     default: 14941
    -M <int>      pixel masking: -1=enable(default), 0=disable, 1=reset
    -d            upload into databse
    -l            unsave log

EOF
}

while getopts m:s:r:n:i:c:t:p:M:dl OPT
do
    case ${OPT} in
        m ) mod_id=${OPTARG} ;;
        s ) scan_type=${OPTARG} ;;
        r ) controller=${OPTARG} ;;
        n ) connectivity=${OPTARG} ;;
        i ) run_info=${OPTARG} ;;
        c ) target_charge=${OPTARG} ;;
        t ) target_tot=${OPTARG} ;;
        p ) target_preamp=${OPTARG} ;;
        M ) mask=${OPTARG} ;;
        d ) database=true ;;
        l ) getlog=false ;;
        * ) usage
            exit ;;
    esac
done

# log
if "${getlog}";then
    mkdir -p log
    LOGFILE="log/${mod_id}_${scan_type}."`date "+%Y%m%d_%H%M%S"`
    exec 2>&1> >(awk '{print strftime("[%Y-%m-%d %H:%M:%S]"),$0 }{ fflush()}' | tee $LOGFILE)
fi

# serial number
if [ -z ${mod_id} ]; then
    echo "Please give serial number with '-m'."
    usage
    exit
fi

# scan
if [ -z ${scan_type} ]; then
    echo "Please give scan type with '-s'."
    usage
    exit
fi

# controller config
if [ -z ${controller} ]; then
    controller=configs/${asic_type}/${mod_id}/controller.json
fi

if [ ! -f ${controller} ]; then
    echo "Not exist controller \"${controller}\"."
    usage
    exit
fi

# connectivity config
if [ -z ${connectivity} ]; then
    connectivity=configs/${asic_type}/${mod_id}/connectivity.json
fi

if [ ! -f ${connectivity} ]; then
    echo "Not exist connectivity \"${connectivity}\"."
    usage
    exit
fi

# environmental config
if "${database}"; then
    if [ -z ${run_info} ]; then
        run_info=configs/${asic_type}/${mod_id}/info.json
    fi
    if [ ! -f ${run_info} ]; then
        echo "Not exist environmental config file \"${run_info}\"."
        usage
        exit
    fi
fi

# set target charge/preamp/tot
echo ${target_charge} | grep [^0-9] > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "Please give an integral as target charge with '-c'. "
    usage
    exit
fi
echo ${target_preamp} | grep [^0-9] > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "Please give an integral as target preamp with '-p'. "
    usage
    exit
fi
echo ${target_tot} | grep [^0-9] > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "Please give an integral as target tot with '-t'. "
    usage
    exit
fi
if echo "${scan_type}" | grep "preamp" >>/dev/null; then
    target_amp_or_charge=${target_preamp}
else
    target_amp_or_charge=${target_charge}
fi

# scanConsole
if ${database}; then
    echo "./bin/scanConsole -r ${controller} -c ${connectivity} -p -t ${target_amp_or_charge} ${target_tot} -s ${scan_type} -m ${mask} -W -I ${run_info}" 
    ./bin/scanConsole -r ${controller} -c ${connectivity} -p -t ${target_amp_or_charge} ${target_tot} -s ${scan_type} -m ${mask} -W -I ${run_info} 
else
    echo "./bin/scanConsole -r ${controller} -c ${connectivity} -p -t ${target_amp_or_charge} ${target_tot} -s ${scan_type} -m ${mask}"
    ./bin/scanConsole -r ${controller} -c ${connectivity} -p -t ${target_amp_or_charge} ${target_tot} -s ${scan_type} -m ${mask}
fi
