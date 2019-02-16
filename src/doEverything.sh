#!/bin/bash
#################################
# Contacts: Eunchong Kim (kim@hep.phys.titech.ac.jp)
# Contacts: Arisa Kubota (akubota@hep.phys.titech.ac.jp)
# Conracts: Shohei Yamagaya(yamagaya@champ.hep.sci.osaka-u.ac.jp)
# Project: Yarr
# Description: Run QA scan
# Usage: ./doEverything.sh [-a <rd53a/fei4b>*] [-m <SerialNumber>*] [-i <Environment>] [-r <Controller>] [-c <ChipNum>] [-d] [-R]
# * is required parameter
# Date: Feb 2019
################################

mkdir -p log
LOGFILE="log/${mod_id}_doEverything."`date "+%Y%m%d_%H%M%S"`
exec 2>&1> >(awk '{print strftime("[%Y-%m-%d %H:%M:%S]"),$0 }{ fflush()}' | tee $LOGFILE)

DEBUG=false
if "${DEBUG}"; then
    echo "DEBUG: ON"
fi

database_status="off"
database=""
reset=true
#asic_type=fei4b
asic_type=rd53a

function usage {
    cat <<EOF

Usage:
    ./$(basename ${0}) [-a <rd53a/fei4b>**] [-m SN**] [-i Environment*] [-r Controller] [-c Chips*] [-d] 

Options:
    -a <rd53a/fei4b> asic type (**req.)
    -m <str>         serial number (**req.)
    -i <path>        environmental config (*req.)  default: ./configs/${asic_type}/\${mod_id}/info.json
    -r <path>        controller config             default: ./configs/${asic_type}/\${mod_id}/controller.json or ./configs/controller/specCfg.json (*)
    -c <int>         number of chips (*req.)
    -d               upload into databse

** is required parameter
*  is required parameter in first creation of config

EOF
}

while getopts a:m:i:r:n:c:dR OPT
do
    case ${OPT} in
        a ) asic_type=${OPTARG} ;;
        m ) mod_id=${OPTARG} ;;
        i ) run_info=${OPTARG} ;;
        r ) controller=${OPTARG} ;;
        c ) chips=${OPTARG} ;;
        d ) database="-d" ;;
        R ) reset=false ;;
        * ) usage
            exit ;;
    esac
done

# asic type
if "${DEBUG}"; then
    echo "DEBUG: Check ASIC type: ${asic_type}"
fi
if [ -z ${asic_type} ]; then
    echo "Please give \"rd53a\" or \"fei4b\" with '-a'."
    usage
    exit
elif [ ${asic_type} == "rd53a" ]; then
    chiptype="RD53A"
    # default parameters for FEI4B
    target_tot=8
    target_preamp=10000
    target_charge=1000
    first_target_charge=2000
elif [ ${asic_type} == "fei4b" ]; then
    chiptype="FEI4B"
    # default parameters for FEI4B
    target_tot=9
    target_preamp=14941
    target_charge=2668
else
    echo "Please give \"rd53a\" or \"fei4b\" with '-a'."
    usage
    exit
fi

# serial number
if "${DEBUG}"; then
    echo "DEBUG: Check Serial number: ${mod_id}"
fi
if [ -z ${mod_id} ]; then
    echo "Please give serial number with '-m'."
    usage
    exit
fi

# first creation of config directory
if "${DEBUG}"; then
    echo "DEBUG: Check first creation config:"
    if [ ! -f configs/${asic_type}/${mod_id}/connectivity.json ]; then
        echo "DEBUG:     Continue to create."
    else
        echo "DEBUG:     Already exist."
    fi
fi
if [ ! -f configs/${asic_type}/${mod_id}/connectivity.json ]; then
    echo "Not exist connecitivity config configs/${asic_type}/${mod_id}/connectivity.json"
    echo "Continue to make config directory ... [y/n] "
    read answer
    if [ "${answer}" = "n" ]; then
        exit
    elif [ "${answer}" != "y" ]; then
        echo "Unexpected token ${answer}"
        exit
    fi

    # chips
    if "${DEBUG}"; then
        echo "DEBUG: Check number of chips: ${chips}"
    fi
    if [ -z ${chips} ]; then
        echo "Please give the number of chips with '-c'."
        usage
        exit
    fi
    echo ${chips} | grep [^0-9] > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo "Please give an integral as number of chips with '-c'. "
        usage
        exit
    fi

    # user config
    if "${DEBUG}"; then
        echo "DEBUG: Check user config: ${run_info}"
    fi
    if [ -z ${run_info} ]; then
        echo "Please give user config with '-i'."
        usage
        exit
    elif [ ! -f ${run_info} ]; then
        echo "Not exist user config \"${run_info}\"."
        usage
        exit
    fi

    # controller config
    if "${DEBUG}"; then
        echo "DEBUG: Check controller config: ${controller}"
    fi
    if [ -z ${controller} ]; then
        controller=configs/controller/specCfg.json
    fi
    if [ ! -f ${controller} ]; then
        echo "Not exist controller \"${controller}\"."
        usage
        exit
    fi

    # create config
    cd configs
        if "${DEBUG}"; then
            echo "./create_config.sh -a ${asic_type} -m ${mod_id} -i ${run_info#*/} -c ${chips} -r ${controller#*/} ${database}"
        fi
        ./create_config.sh -a ${asic_type} -m ${mod_id} -i ${run_info#*/} -c ${chips} -r ${controller#*/} ${database}
    cd ../
    reset=false
    controller=configs/${asic_type}/${mod_id}/controller.json
    run_info=configs/${asic_type}/${mod_id}/info.json
fi

# connectivity config
connectivity=configs/${asic_type}/${mod_id}/connectivity.json
if "${DEBUG}"; then
    echo "DEBUG: Check connectivity: ${connectivity}"
fi
if [ ! -f ${connectivity} ]; then
    echo "Not exist connectivity \"${connectivity}\"."
    usage
    exit
fi

# controller config
if "${DEBUG}"; then
    echo "DEBUG: Check controller: ${controller}"
fi
if [ -z ${controller} ]; then
    controller=configs/${asic_type}/${mod_id}/controller.json
fi
if [ ! -f ${controller} ]; then
    echo "Not exist controller \"${controller}\"."
    usage
    exit
fi

# encironmental config
if "${DEBUG}"; then
    echo "DEBUG: Check environmental config: ${run_info}"
fi
if [ -z ${run_info} ]; then
    run_info=configs/${asic_type}/${mod_id}/info.json
fi
if [ ! -f ${run_info} ]; then
    echo "Not exist environmental config \"${run_info}\"."
    usage
    exit
fi

# database status
if [ "${database}" = "-d" ]; then
    database_status="on"
fi

echo "Now status is ......."
echo "ASIC type            : ${asic_type}"
echo "module ID            : ${mod_id}"
echo "database             : ${database_status}"
if "${DEBUG}"; then
    echo "DEBUG: connectivity  : ${connectivity}"
    echo "DEBUG: controller    : ${controller}"
fi
echo "environmental config : ${run_info}"
sed -n "s/userIdentity/userIdentity/p" ${run_info}
sed -n "s/institution/institution/p" ${run_info}
echo "Continue ... [y/n] "
read answer
if [ "${answer}" = "n" ]; then
    exit
elif [ "${answer}" != "y" ]; then
    echo "Unexpected token ${answer}"
    exit
fi

# reset config
if "${DEBUG}"; then
    echo "DEBUG: Check reset config:"
    if "${reset}"; then
        echo "DEBUG:     Continue to reset."
    else
        echo "DEBUG:     Already reseted."
    fi
fi
if "${reset}"; then
    cd configs
        if [ "${database}" = "-d" ]; then
            if "${DEBUG}"; then
                echo "./create_config.sh -a ${asic_type} -m ${mod_id} -i ${run_info#*/} -r ${controller#*/} -d"
            fi
            ./create_config.sh -a ${asic_type} -m ${mod_id} -i ${run_info#*/} -r ${controller#*/} -d
        else 
            if "${DEBUG}"; then
                echo "./create_config.sh -a ${asic_type} -m ${mod_id} -i ${run_info#*/} -r ${controller#*/} -d"
            fi
            ./create_config.sh -a ${asic_type} -m ${mod_id} -i ${run_info#*/} -r ${controller#*/}
        fi
    cd ../
fi

if [ "${asic_type}" = "fei4b" ]; then
    scan_list=(
        digitalscan
        thresholdscan
        tune_globalthreshold
        thresholdscan
        tune_pixelthreshold
        thresholdscan
        tune_globalpreamp
        totscan
        tune_pixelpreamp
        totscan
        digitalscan
        analogscan
        thresholdscan
        noisescan
    )
    if "${DEBUG}"; then
        echo "DEBUG: Check scan list:"
        for scan_type in ${scan_list[@]};
        do
            echo ${scan_type}
        done
    fi
elif [ "${asic_type}" = "rd53a" ]; then
    scan_list=(
        std_digitalscan
        std_analogscan
        diff_tune_globalthreshold
        diff_tune_pixelthreshold
        diff_tune_globalpreamp
        diff_tune_pixelthreshold
        lin_tune_globalthreshold
        lin_tune_pixelthreshold
        lin_retune_globalthreshold
        lin_retune_pixelthreshold
        lin_tune_globalpreamp
        lin_retune_pixelthreshold
        syn_tune_globalthreshold
        syn_tune_globalpreamp
        syn_tune_globalthreshold
        std_thresholdscan
        std_totscan
        std_noisescan
    )
    if "${DEBUG}"; then
        echo "DEBUG: Check scan list:"
        for scan_type in ${scan_list[@]};
        do
            echo "DEBUG:     ${scan_type}"
        done
    fi
fi
for scan_type in ${scan_list[@]};
do
    if [ "${asic_type}" = "rd53a" ]; then
        if [ `echo "${scan_type}" | grep -v "std_digitalscan" | grep -v "std_analogscan" | grep -v "std_noisescan"` ];then
            mask="-M 0"
        fi
    fi
    if [ `echo "${scan_type}" | grep "preamp"` ];then
        target_c="-p ${target_preamp}"
        target_t="-t ${target_tot}"
    elif [ `echo "${scan_type}" | grep "tot"` ];then
        target_c="-p ${target_preamp}"
        target_t="-t ${target_tot}"
    elif [ `echo "${scan_type}" | grep "lin_tune_globalthreshold"` ]; then
        target_c="-c ${first_target_charge}"
    elif [ `echo "${scan_type}" | grep "lin_tune_pixelthreshold"` ]; then
        target_c="-c ${first_target_charge}"
    elif [ `echo "${scan_type}" | grep "threshold"` ];then
        target_c="-c ${target_charge}"
    fi
    if "${DEBUG}"; then
        echo "DEBUG: Check target of ${scan_type}: ${target_c}(c), ${target_t}(t)"
    fi
    echo "Starting ${scan_type} ......"
    if "${DEBUG}"; then
        echo "./doScan_${asic_type}.sh -m ${mod_id} -i ${run_info} -r ${controller} -n ${connectivity} ${database} -s ${scan_type} ${target_c} ${target_t} ${mask} -l"
    fi
    ./doScan_${asic_type}.sh -m ${mod_id} -i ${run_info} -r ${controller} -n ${connectivity} ${database} -s ${scan_type} ${target_c} ${target_t} ${mask} -l
    unset target_c
    unset target_t
    unset mask 
done

