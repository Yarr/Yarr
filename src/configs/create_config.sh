#!/bin/bash
#################################
# Contacts: Eunchong Kim, Arisa Kubota
# Email: eunchong.kim at cern.ch, arisa.kubota at cern.ch
# Date: April 2019
# Project: Local Database for Yarr
# Description: Create config file 
# Usage: ./create_config.sh [-a <rd53a/fei4b>*] [-m <SerialNumber>*] [-c <ChipNum>] [-i <UserCfg>] [-r <ControllerCfg>] [-d] [-R]
################################

# Change fixed tx channel and rx start channel
tx_fix=0
rx_start=0

# default parameters
database=false
reset=false
now=`date +"%y%m%d%H%M"`

# Usage
function usage {
    cat <<EOF

Usage:
    ./$(basename ${0}) [-a <rd53a/fei4b>*] [-m SN*] [-c Chips] [-i User] [-r Controller] [-d] [-R]

Options:
    -a <rd53a/fei4b> asic type (*req.)
    -m <str>         serial number (*req.)
    -c <int>         number of chips (req. in first creation)
    -i <path>        user config file (req. in first creation) 
    -r <path>        controller config file   default: ./controller/specCfg.json
    -d               upload into databse
    -R               reset all config files

EOF
}

while getopts a:m:c:i:r:dR OPT
do
    case ${OPT} in
        a ) asic=${OPTARG} ;;
        m ) sn=${OPTARG} ;;
        c ) chips=${OPTARG} ;;
        i ) user=${OPTARG} ;;
        r ) controller=${OPTARG} ;;
        d ) database=true ;;
        R ) reset=true ;;
        * ) usage
            exit ;;
    esac
done

# asic type
if [ -z ${asic} ]; then
    echo "Please give \"rd53a\" or \"fei4b\" with '-a'."
    usage
    exit
elif [ ${asic} == "rd53a" ]; then
    chiptype="RD53A"
    chipid="ChipId"
    name="Name"
    john="JohnDoe"
elif [ ${asic} == "fei4b" ]; then
    chiptype="FEI4B"
    chipid="chipId"
    name="name"
    john="JohnDoe"
else
    echo "Please give \"rd53a\" or \"fei4b\" with '-a'."
    usage
    exit
fi

# serial number
if [ -z ${sn} ]; then
    echo "Please give serial number with '-m'."
    usage
    exit
fi

# reset directory
if "${reset}"; then
    echo "Reset config files of ${sn}."
    if [ -d ${asic}/${sn} ]; then
        echo "Move config files to backup directory before reset."
        mkdir -p ${asic}/${sn}/backup/${now}
        rsync -a ${asic}/${sn}/ ${asic}/${sn}/backup/${now} --exclude '/backup/'
        if [ -f ${asic}/${sn}/connectivity.json ]; then
            rm ${asic}/${sn}/connectivity.json
            rm ${asic}/${sn}/chip*.json
        else
            echo "Not exist connectivity.json and nothing was done."
        fi
    else
        echo "Not exist config files of ${sn} and nothing was done."
    fi
    echo "Continue to remake config files ... [y/n] "
    read answer
    if [ "${answer}" == "n" ]; then
        exit
    elif [ "${answer}" != "y" ]; then
        echo "Unexpected token ${answer}"
        exit
    fi
fi

# Make directory for config file
if [ -f ${asic}/${sn}/connectivity.json ]; then
    echo "Config files of ${sn} are already exist, then move it to backup directory"
    mkdir -p ${asic}/${sn}/backup/${now}
    rsync -a ${asic}/${sn}/ ${asic}/${sn}/backup/${now} --exclude '/backup/'

    # Make user config 
    if [ ! -z ${user} ]; then
        if [ ${user} != ${asic}/${sn}/info.json ]; then
            echo "Create info.json"
            cp ${user} ${asic}/${sn}/info.json
        fi
    fi

    # Make controller for this module
    if [ ! -z ${controller} ]; then
        if [ ${controller} != ${asic}/${sn}/controller.json ]; then
            echo "Create controller.json"
            cp ${controller} ${asic}/${sn}/controller.json
        fi
    fi

else    # first creation
    # chips
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
    if [ -z ${user} ]; then
        echo "Please give user config file with '-i'."
        usage
        exit
    elif [ ! -f ${user} ]; then
        echo "Not exist user config file \"${user}\"."
        usage
        exit
    fi

    # controller config
    if [ -z ${controller} ]; then
        controller=controller/specCfg.json
    fi
    if [ ! -f ${controller} ]; then
        echo "Not exist controller \"${controller}\"."
        usage
        exit
    fi

    # Make config directory
    if [ ! -d ${asic}/${sn} ]; then
        echo "Create config directory ${asic}/${sn}"
        mkdir -p ${asic}/${sn}/backup
    fi

    # Make controller for this module
    if [ ${controller} != ${asic}/${sn}/controller.json ]; then
        echo "Create controller.json"
        cp ${controller} ${asic}/${sn}/controller.json
    fi
    
    # Make user config 
    if [ ${user} != ${asic}/${sn}/info.json ]; then
        echo "Create info.json"
        cp ${user} ${asic}/${sn}/info.json
    fi
fi

# Make config file for each chip and connectivity
if [ ! -f ${asic}/${sn}/connectivity.json ]; then
    echo "Create connectivity.json"
    echo "{" > ${asic}/${sn}/connectivity.json
    echo "    \"module\": {" >> ${asic}/${sn}/connectivity.json
    echo "        \"serialNumber\": \"${sn}\"," >> ${asic}/${sn}/connectivity.json
    echo "        \"componentType\": \"Module\"" >> ${asic}/${sn}/connectivity.json
    echo "    }," >> ${asic}/${sn}/connectivity.json
    echo "    \"chipType\" : \"${chiptype}\"," >> ${asic}/${sn}/connectivity.json

    cnt=0
    echo "    \"chips\" : [" >> ${asic}/${sn}/connectivity.json
    while [ ${cnt} -lt ${chips} ]; do
        cnt=$(( cnt + 1 ))
        echo "Create chip${cnt}.json"
        echo "---------------------"
        echo "name: ${sn}_chip${cnt}"
        echo "chipId: ${cnt}"
        echo "---------------------"
        echo "Change name ... [<chipName>/n] "
        read answer
        if [ "${answer}" != "n" ]; then
            chipName=${answer}
        else
            chipName="chip${cnt}"
        fi
        echo "Change chipId ... [#(chipId)/n] "
        read answer
        if [ "${answer}" != "n" ]; then
            echo ${answer} | grep [^0-9] > /dev/null 2>&1
            if [ $? -eq 0 ]; then
                echo "Please give an integral as chipId. "
                exit
            else
                id=${answer}
            fi
        else
            id=${cnt}
        fi

        echo "Create ${chipName}.json"
        cp defaults/default_${asic}.json ${asic}/${sn}/${chipName}.json
        sed -i "/${chipid}/s/0/${id}/g" ${asic}/${sn}/${chipName}.json
        sed -i "/${name}/s/JohnDoe/${chipName}/g" ${asic}/${sn}/${chipName}.json
    
        rx_ch=$(( cnt - 1 + rx_start ))
        echo "        {" >> ${asic}/${sn}/connectivity.json
        echo "            \"serialNumber\": \"${sn}_${chipName}\"," >> ${asic}/${sn}/connectivity.json
        echo "            \"componentType\": \"Front-end Chip\"," >> ${asic}/${sn}/connectivity.json
        echo "            \"config\" : \"configs/${asic}/${sn}/${chipName}.json\"," >> ${asic}/${sn}/connectivity.json
        echo "            \"tx\" : ${tx_fix}," >> ${asic}/${sn}/connectivity.json
        echo "            \"rx\" : ${rx_ch}" >> ${asic}/${sn}/connectivity.json
        if [ ${cnt} -ne ${chips} ]; then
            echo "        }," >> ${asic}/${sn}/connectivity.json
        else
            echo "        }" >> ${asic}/${sn}/connectivity.json
        fi
    done
    echo "    ]" >> ${asic}/${sn}/connectivity.json
    echo "}" >> ${asic}/${sn}/connectivity.json
else
    for file in `\find ${asic}/${sn} -path "${asic}/${sn}/chip*.json"` ; do
        cfgname=`cat ${file} | grep ${name} | awk -F'["]' '{print $4}'`
        cfgid=`cat ${file} | grep \"${chipid}\" | awk '{print $2}'`
        echo "Reset ${file##*/}"
        cp defaults/default_${asic}.json ${file}
        sed -i "/${chipid}/s/0,/${cfgid}/g" ${file}
        sed -i "/${name}/s/JohnDoe/${cfgname}/g" ${file}
    done
fi

cd ../

# Register module and chips component to DB
if "${database}"; then
    echo "./bin/dbRegister -C configs/${asic}/${sn}/connectivity.json "
    ./bin/dbRegister -C configs/${asic}/${sn}/connectivity.json
fi
