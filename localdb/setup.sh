#!/bin/bash
#################################
# Contacts: Arisa Kubota
# Email: arisa.kubota at cern.ch
# Date: April 2019
# Project: Local Database for Yarr
# Description: Login Database 
# Usage: ./setup.sh [Local DB server ip (default: 127.0.0.1)] [Local DB server port (default: 27017)]
################################

DEBUG=false

# Usage
function usage {
    cat <<EOF

Usage:
    ./setup.sh [ip address] [port] [dir path]

Options:
    - ip address  Local DB server ip address, default: 127.0.0.1
    - port        Local DB server port, default: 27017
    - dir path    path to Local DB cache directory, default: Yarr/src/cacheDB

EOF
}

if [ ! -e ${HOME}/.yarr ]; then
    mkdir ${HOME}/.yarr
fi

ip=$1
port=$2
dir=$3

if [ -z ${ip} ]; then
    ip=127.0.0.1
fi
if [ -z ${port} ]; then
    port=27017
fi
if [ -z ${dir} ]; then
    cd ../
    current=`pwd`
    dir=${current}/src/cacheDB
fi

# create database config
dbcfg=${HOME}/.yarr/database.json
echo "{" > ${dbcfg}
echo "    \"hostIp\": \"${ip}\"," >> ${dbcfg}
echo "    \"hostPort\": \"${port}\"," >> ${dbcfg}
echo "    \"cache\": \"${dir}\"," >> ${dbcfg}
echo "    \"stage\": [" >> ${dbcfg}
echo "        \"Bare Module\"," >> ${dbcfg}
echo "        \"Wire Bonded\"," >> ${dbcfg}
echo "        \"Potted\"," >> ${dbcfg}
echo "        \"Final Electrical\"," >> ${dbcfg}
echo "        \"Complete\"," >> ${dbcfg}
echo "        \"Loaded\"," >> ${dbcfg}
echo "        \"Parylene\"," >> ${dbcfg}
echo "        \"Initial Electrical\"," >> ${dbcfg}
echo "        \"Thermal Cycling\"," >> ${dbcfg}
echo "        \"Flex + Bare Module Attachment\"," >> ${dbcfg}
echo "        \"Testing\"" >> ${dbcfg}
echo "    ]," >> ${dbcfg}
echo "    \"environment\": [" >> ${dbcfg}
echo "        \"vddd_voltage\"," >> ${dbcfg}
echo "        \"vddd_current\"," >> ${dbcfg}
echo "        \"vdda_voltage\"," >> ${dbcfg}
echo "        \"vdda_current\"," >> ${dbcfg}
echo "        \"hv_voltage\"," >> ${dbcfg}
echo "        \"hv_current\"," >> ${dbcfg}
echo "        \"temperature\"" >> ${dbcfg}
echo "    ]," >> ${dbcfg}
echo "    \"component\": [" >> ${dbcfg}
echo "        \"Front-end Chip\"," >> ${dbcfg}
echo "        \"Front-end Chips Wafer\"," >> ${dbcfg}
echo "        \"Hybrid\"," >> ${dbcfg}
echo "        \"Module\"," >> ${dbcfg}
echo "        \"Sensor Tile\"," >> ${dbcfg}
echo "        \"Sensor Wafer\"" >> ${dbcfg}
echo "    ]" >> ${dbcfg}
echo "}" >> ${dbcfg}
echo "Create DB Config file: ${dbcfg}"
echo " "

# create site address config 
address=${HOME}/.yarr/address.json
unset institution
declare -a nic=()  
num=0
for DEV in `find /sys/devices -name net | grep -v virtual`; 
do 
    nic[${num}]=`ls --color=none ${DEV}`
    num=$(( num + 1 ))
done
dbnic="${nic[0]}"
if [ -f ${address} ]; then
    tmpmacaddress=`cat ${address}|grep 'macAddress'|awk -F'["]' '{print $4}'`
    tmpname=`cat ${address}|grep 'name'|awk -F'["]' '{print $4}'`
    tmpinstitution=`cat ${address}|grep 'institution'|awk -F'["]' '{print $4}'`
fi
macaddress=`ifconfig ${dbnic} | grep -o -E '([[:xdigit:]]{1,2}:){5}[[:xdigit:]]{1,2}'`
if [ -f ${address} ] && [ ${macaddress} = ${tmpmacaddress} ] && [ ${HOSTNAME} = ${tmpname} ] && [ -n ${tmpinstitution} ]; then
    echo "Site Config file is exist: ${address}"
    institution=${tmpinstitution}
else
    echo "Enter the institution (ABC Laboratory) where this machine (MAC address: ${macaddress}) is or 'exit' ... "
    read -p "> " -a answer
    while [ ${#answer[@]} == 0 ]; 
    do
        echo "Enter the institution (ABC Laboratory) where this machine (MAC address: ${macaddress}) is or 'exit' ... "
        read -p "> " -a answer
    done
    if [ ${answer[0]} == "exit" ]; then
        echo "Exit ..."
    else
        for a in ${answer[@]}; do
            institution="${institution#_}_${a}"
        done
    fi
fi

echo " "
echo "Test Site Information"
echo "  MAC address: ${macaddress}"
echo "  Machine Name: ${HOSTNAME}"   
echo "  Institution: ${institution}"
echo " "
echo "Are you sure that's correct? [y/n]"
read -p "> " answer
while [ -z ${answer} ]; 
do
    echo "Are you sure that's correct? [y/n]"
    read -p "> " answer
done
echo " "

if [ ${answer} != "y" ]; then
    echo "Exit ..."
    echo " "
fi

echo "{" > ${address}
echo "    \"macAddress\": \"${macaddress}\"," >> ${address}
echo "    \"name\": \"${HOSTNAME}\"," >> ${address}
echo "    \"institution\": \"${institution}\"" >> ${address}
echo "}" >> ${address}
echo "Create Site Config file: ${address}"
echo " "

# create cache directory

if [ ! -e ${dir} ]; then
    mkdir -p ${dir}/lib ${dir}/var/log ${dir}/tmp
    mkdir -p ${dir}/
fi

echo "Create Cache Directory: ${dir}"
echo " "
