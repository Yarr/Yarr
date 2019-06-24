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

#daemon=false

while getopts i:p:c:n:d OPT
do
    case ${OPT} in
        i ) ip=${OPTARG} ;;
        p ) port=${OPTARG} ;;
        c ) dir=${OPTARG} ;;
        n ) dbname=${OPTARG} ;;
#        d ) daemon=true ;;
        * ) usage
            exit ;;
    esac
done

#if ${daemon}; then
#    read -sp "[sudo] password: " password
#    echo " "
#fi

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
    cd ../
    yarrDir=`pwd`
    dir=${yarrDir}/localdb/cacheDB
fi

# create database config
dbcfg=${HOME}/.yarr/${HOSTNAME}_database.json
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
address=${HOME}/.yarr/${HOSTNAME}_address.json
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

# create cache directory

if [ ! -e ${dir} ]; then
    mkdir -p ${dir}
fi
if [ ! -e ${dir}/lib/tmp ]; then
    mkdir -p ${dir}/lib/tmp
fi
if [ ! -e ${dir}/var/log ]; then
    mkdir -p ${dir}/var/log
fi
if [ ! -e ${dir}/var/cache ]; then
    mkdir -p ${dir}/var/cache/scan
    mkdir -p ${dir}/var/cache/db
    mkdir -p ${dir}/var/cache/dcs
fi
if [ ! -e ${dir}/tmp/scan ]; then
    mkdir -p ${dir}/tmp/scan
fi

echo "Create Cache Directory: ${dir}"
echo " "

echo "MongoDB Server IP address: ${ip}, port: ${port}"
echo " "

#if ${daemon}; then
#    cd ${yarrDir}/src
#    #make clean
#    make
#    
#    cp ${yarrDir}/src/bin/dbAccessor ${yarrDir}/localdb/dbAccessor
#    echo "Copy dbAccessor"
#    echo " "
#
#    cd ${yarrDir}/localdb
#    ./dbAccessor -S
#    if [ $? == 1 ]; then
#        echo "MongoDB connection is failed!"
#    fi
#fi
