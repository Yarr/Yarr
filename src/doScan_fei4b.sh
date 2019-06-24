#!/bin/bash
#################################
# Contacts: Eunchong Kim, Arisa Kubota, Shohei Yamagaya
# Email: eunchong.kim at cern.ch, arisa.kubota at cern.ch, yamagaya at champ.hep.sci.osaka-u.ac.jp
# Date: April 2019
# Project: Local Database for Yarr
# Description: Run scan
# Usage: ./doScan_fei4b.sh [-m <SerialNumber>*] [-s <ScanType>*] [-r <ControllerCfg>] [-n <ConnecitiityCfg>] [-i <EnvironmentalCfg>] [-c <TargetCharge>] [-t <TargetTot>] [-p <TargetPreamp>] [-M <mask>] [-d <account>] [-l]
# * is required item
################################

# default parameters
asic_type=fei4b
target_tot=9
target_preamp=14941
target_charge=2668
mask=-1
getlog=false

function usage {
    cat <<EOF

Usage:
    ./$(basename ${0}) [-m SN*] [-s Scan*] [-r Controller] [-n Connectivity] [-c Charge] [-t Tot] [-p Preamp] [-M mask] [-d db user account] [-l]

Options:
    -m <str>      serial number (*req.)
    -s <str/path> scan type (*req.)
    -r <path>     controller config file            default: ./configs/${asic_type}/\${mod_id}/controller.json
    -n <path>     connectivity config file          default: ./configs/${asic_type}/\${mod_id}/connectivity.json
    -c <int>      target charge for tune_threshold  default: 2668
    -t <int>      target tot for tune_preamp        default: 9
    -p <int>      target preamp for tune_preamp     default: 14941
    -M <int>      pixel masking: -1=enable(default), 0=disable, 1=reset
    -d <str>      upload into databse
    -I <str>      database config file              default: ${HOME}/.yarr/${HOSTNAME}_database.json
    -l            unsave log

EOF
}

while getopts m:s:r:n:c:t:p:M:d:lI: OPT
do
    case ${OPT} in
        m ) mod_id=${OPTARG} ;;
        s ) scan_type=${OPTARG} ;;
        r ) controller=${OPTARG} ;;
        n ) connectivity=${OPTARG} ;;
        c ) target_charge=${OPTARG} ;;
        t ) target_tot=${OPTARG} ;;
        p ) target_preamp=${OPTARG} ;;
        M ) mask=${OPTARG} ;;
        d ) db_account=${OPTARG} ;;
        l ) getlog=false ;;
        I ) database=${OPTARG} ;;
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
if [ `echo ${scan_type} | grep "json"` ]; then
    if [ ! -f ${scan_type} ]; then
        echo "Not exist scan \"${scan_type}\"."
        usage
        exit
    fi
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
if [ ! -z ${db_account} ]; then
    if [ ! -z ${database} ]; then
        if [ ! -f ${database} ]; then
            echo "Not exist database config file \"${database}\"."
            usage
            exit
        fi
        database="-I ${database}"
    fi
    db_account="-W ${db_account}"
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
echo "./bin/scanConsole -r ${controller} -c ${connectivity} -p -t ${target_amp_or_charge} ${target_tot} -s ${scan_type} -m ${mask} ${db_account} ${database}"
./bin/scanConsole -r ${controller} -c ${connectivity} -p -t ${target_amp_or_charge} ${target_tot} -s ${scan_type} -m ${mask} ${db_account} ${database}
