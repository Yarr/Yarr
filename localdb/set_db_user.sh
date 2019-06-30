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
    ./setup.sh [-i ip address] [-p port] [-c dir path] [-d]

Options:
    - i <ip address>  Local DB server ip address, default: 127.0.0.1
    - p <port>        Local DB server port, default: 27017
    - n <db name>     Local DB Name, default: localdb
    - c <dir path>    path to Local DB cache directory, default: Yarr/localdb/cacheDB

EOF
}

if [ ! -e ${HOME}/.yarr ]; then
    mkdir ${HOME}/.yarr
fi

while getopts i:p:c:n:d OPT
do
    case ${OPT} in
        i ) ip=${OPTARG} ;;
        p ) port=${OPTARG} ;;
        c ) dir=${OPTARG} ;;
        n ) dbname=${OPTARG} ;;
        * ) usage
            exit ;;
    esac
done

cd ../
yarrDir=`pwd`
cd - > /dev/null

if [ -z ${ip} ]; then
    ip=127.0.0.1
fi
if [ -z ${port} ]; then
    port=27017
fi
if [ -z ${dbname} ]; then
    dbname="localdb"
fi
if [ -z ${dir} ]; then
    dir=${HOME}/.yarr/localdb
fi

# create cache directory

if [ ! -e ${dir} ]; then
    mkdir -p ${dir}
fi
if [ ! -e ${dir}/var ]; then
    mkdir -p ${dir}/var/cache/scan
    mkdir -p ${dir}/var/log/db
    mkdir -p ${dir}/var/cache/dcs
    mkdir -p ${dir}/lib/tmp
    mkdir -p ${dir}/tmp/db
    mkdir -p ${dir}/etc
fi

echo "Created Cache Directory: ${dir}"
echo " "

# create database config
dbcfg=${dir}/etc/${USER}_database.json
echo "{" > ${dbcfg}
echo "    \"hostIp\": \"${ip}\"," >> ${dbcfg}
echo "    \"hostPort\": \"${port}\"," >> ${dbcfg}
echo "    \"dbName\": \"${dbname}\"," >> ${dbcfg}
echo "    \"cachePath\": \"${dir}\"," >> ${dbcfg}
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
address=${dir}/etc/${USER}_address.json
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
    echo "Enter the institution name where this machine (MAC address: ${macaddress}) is or 'exit' ... "
    read -p "> " -a answer
    while [ ${#answer[@]} == 0 ]; 
    do
        echo "Enter the institution name where this machine (MAC address: ${macaddress}) is or 'exit' ... "
        read -p "> " -a answer
    done
    if [ ${answer[0]} == "exit" ]; then
        echo "Exit ..."
        echo " "
        exit
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
    if [ -f ${address} ]; then 
        echo "Remove Site Config file: ${address}"
        echo " "
        rm ${address}
    fi
    echo "Try again setup.sh, Exit ..."
    echo " "
    exit
fi

echo "{" > ${address}
echo "    \"macAddress\": \"${macaddress}\"," >> ${address}
echo "    \"hostname\": \"${HOSTNAME}\"," >> ${address}
echo "    \"institution\": \"${institution}\"" >> ${address}
echo "}" >> ${address}
echo "Create Site Config file: ${address}"
echo " "

echo "Created Cache Directory: ${dir}"
echo " "

echo "MongoDB Server IP address: ${ip}, port: ${port}"
echo " "


cfg=${dir}/etc/${USER}_user.json

echo "{" > ${cfg}
echo "    \"userName\": \"${USER}\"," >> ${cfg}
echo "    \"institution\": \"${HOSTNAME}\"," >> ${cfg}
echo "    \"userIdentity\": \"default\"" >> ${cfg}
echo "}" >> ${cfg}
echo "Create User Config file: ${cfg}"
echo " "

echo ""
echo "Finished installation!!"
echo "Install log can be found in: $LOGFILE"
echo ""
echo "----------------------------------------------------------------"
echo "-- First thing to do..."
echo "----------------------------------------------------------------"
echo "Scan with uploading Local DB by..." | tee README
echo "cd ../src" | tee -a README
echo "./bin/scanConsole -c configs/connectivity/example_rd53a_setup.json -r configs/controller/specCfg.json -s configs/scans/rd53a/std_digitalscan.json -W -u ${USER} -d ${dbcfg} -i ${address}" | tee -a README
echo "./bin/dbAccessor -R ~/.yarr/localdb/var/cache/timestamp_directory" | tee -a README
echo "" | tee -a README
echo "Check results in the DB viewer in your web browser..." | tee -a README
echo "From the DAQ machine: http://localhost:5000/localdb/" | tee -a README
echo "From other machines : http://${ip}/localdb/" | tee -a README
echo "" | tee -a README
echo "Check more detail at..." | tee -a README
echo "" | tee -a README
echo "https://github.com/jlab-hep/Yarr/wiki/Quick-tutorial" | tee -a README
echo ""
echo "Prepared a README file for the reminder. Enjoy!!"

