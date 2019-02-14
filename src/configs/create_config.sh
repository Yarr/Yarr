#!/bin/bash
#################################
# Contacts: Eunchong Kim (kim@hep.phys.titech.ac.jp)
# Contacts: Arisa Kubota (akubota@hep.phys.titech.ac.jp)
# Project: Yarr
# Description: Create config file 
# Usage: ./create_config.sh [-a <rd53a/fei4b>] [-m <SerialNumber>] [-i <UserConfig>] [-c <ChipNum>] [-r <Controller>]
# Date: Jan 2019
################################

# Change fixed tx channel and rx start channel
tx_fix=4
rx_start=4

# Usage
function usage {
    cat <<EOF

Usage:
    ./$(basename ${0}) [-a <rd53a/fei4b>] [-m <SerialNumber>] [-i <UserConfig>] [-c <ChipNum>] [-r <Controller>]

EOF
}

# Fill arguments into variable
controller=controller/specCfg.json

while getopts a:m:i:c:r: OPT
do
    case ${OPT} in
        a ) asic=${OPTARG} ;;
        m ) sn=${OPTARG} ;;
        i ) user=${OPTARG} ;;
        c ) chips=${OPTARG} ;;
        r ) controller=${OPTARG} ;;
        * ) usage
            exit ;;
    esac
done

# Check arguments
if [ -z ${asic} ]; then
    echo "Please give \"rd53a\" or \"fei4b\" with '-a'."
    usage
    exit
elif [ ${asic} == "rd53a" ]; then
    chiptype="RD53A"
    chipid="ChipId"
    name="Name"
elif [ ${asic} == "fei4b" ]; then
    chiptype="FEI4B"
    chipid="chipId"
    name="name"
else
    echo "Please give \"rd53a\" or \"fei4b\" with '-a'."
    usage
    exit
fi

if [ -z ${sn} ]; then
    echo "Please give serial number with '-m'."
    usage
    exit
fi

if [ -z ${user} ]; then
    echo "Please give user config file with '-i'."
    usage
    exit
elif [ ! -f ${user} ]; then
    echo "Not exist user config file \"${user}\"."
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

echo "Write Database ... [ asic: ${asic}, serial number: ${sn}, user config: ${user}, number of chips: ${chips} ]"
echo -n "Continue ... [y/n] "
read answer
if [ $answer = "n" ]; then
    exit
fi

# Make directory for config file
echo "Create config directory ${asic}/${sn}"
now=`date +"%y%m%d%H%M"`
if [ ! -d "${asic}" ]; then
    mkdir ${asic}
    mkdir ${asic}/backup
fi
if [ -d "${asic}/${sn}" ]; then
    echo "Config files of ${sn} are already exist, then move it in backup directory"
    mv ${asic}/${sn} ${asic}/backup/${sn}_${now}
fi
mkdir ${asic}/${sn}

# Make config file for each chip and connectivity
echo "Create connectivity.json"
echo "{" > ${asic}/${sn}/connectivity.json
echo "    \"module\": {" >> ${asic}/${sn}/connectivity.json
echo "        \"serialNumber\": \"${sn}\"," >> ${asic}/${sn}/connectivity.json
echo "        \"componentType\": \"Module\"" >> ${asic}/${sn}/connectivity.json
echo "    }," >> ${asic}/${sn}/connectivity.json
echo "    \"chipType\" : \"${chiptype}\"," >> ${asic}/${sn}/connectivity.json

id=0
echo "    \"chips\" : [" >> ${asic}/${sn}/connectivity.json
while [ ${id} -lt ${chips} ]; do
    id=$(( id + 1 ))
    echo "Create chipId${id}.json"
    cp defaults/default_${asic}.json ${asic}/${sn}/chipId${id}.json
    sed -i "/${chipid}/s/0/${id}/g" ${asic}/${sn}/chipId${id}.json
    sed -i "/${name}/s/JohnDoe/chipId${id}/g" ${asic}/${sn}/chipId${id}.json

    rx_ch=$(( id - 1 + rx_start ))
    echo "        {" >> ${asic}/${sn}/connectivity.json
    echo "            \"serialNumber\": \"${sn}_chipId${id}\"," >> ${asic}/${sn}/connectivity.json
    echo "            \"componentType\": \"Front-end Chip\"," >> ${asic}/${sn}/connectivity.json
    echo "            \"config\" : \"configs/${asic}/${sn}/chipId${id}.json\"," >> ${asic}/${sn}/connectivity.json
    echo "            \"tx\" : ${tx_fix}," >> ${asic}/${sn}/connectivity.json
    echo "            \"rx\" : ${rx_ch}" >> ${asic}/${sn}/connectivity.json
    if [ ${id} -ne ${chips} ]; then
        echo "        }," >> ${asic}/${sn}/connectivity.json
    else
        echo "        }" >> ${asic}/${sn}/connectivity.json
    fi
done
echo "    ]" >> ${asic}/${sn}/connectivity.json
echo "}" >> ${asic}/${sn}/connectivity.json

# Make controller for this module
echo "Create controller.json"
cp ${controller} ${asic}/${sn}/controller.json

cd ../

## Register module and chips component to DB
echo "./bin/dbRegisterComponent -c configs/${asic}/${sn}/connectivity.json -I configs/${user}"
./bin/dbRegisterComponent -c configs/${asic}/${sn}/connectivity.json -I configs/${user}
