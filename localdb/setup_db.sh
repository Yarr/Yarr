#!/bin/bash
#################################
# Contacts: Arisa Kubota
# Email: arisa.kubota at cern.ch
# Date: July 2019
# Project: Local Database for Yarr
# Usage: ./setup_db.sh [-i Local DB server ip (default: 127.0.0.1)] [-p Local DB server port (default: 27017)] [-c path to cache directory (default: $HOME/.yarr/localdb) [-n Local DB name (default: localdb)]
################################

DEBUG=false

# Usage
function usage {
    cat <<EOF

Usage:
    ./setup_db.sh [-i ip address] [-p port] [-c dir path] [-n db name]

Options:
    - i <ip address>  Local DB server ip address, default: 127.0.0.1
    - p <port>        Local DB server port, default: 27017
    - c <dir path>    path to Local DB cache directory, default: $HOME/.yarr/localdb
    - n <db name>     Local DB Name, default: localdb

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

# Check the name of site
address=${dir}/etc/address.json
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

# Confirmation
echo " "
echo "MongoDB Server Information"
echo "  IP address: ${ip}"
echo "  port: ${port}"
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
    echo "Try again setup_db.sh, Exit ..."
    echo " "
    exit
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

# create database config
dbcfg=${dir}/etc/database.json
cp default/database.json ${dbcfg}
sed -i -e "s!DBIP!${ip}!g" ${dbcfg}
sed -i -e "s!DBPORT!${port}!g" ${dbcfg}
sed -i -e "s!DBNAME!${dbname}!g" ${dbcfg}
sed -i -e "s!CACHE!${dir}!g" ${dbcfg}
echo "Create DB Config file: ${dbcfg}"

# create site address config 
cp default/address.json ${address}
sed -i -e "s!MACADDR!${macaddress}!g" ${address}
sed -i -e "s!HOSTNAME!${HOSTNAME}!g" ${address}
sed -i -e "s!SITE!${institution}!g" ${address}
echo "Create Site Config file: ${address}"

# create default user config
cfg=${dir}/etc/${USER}_user.json
cp default/user.json ${cfg}
sed -i -e "s!NAME!${USER}!g" ${cfg}
sed -i -e "s!INSTITUTION!${HOSTNAME}!g" ${cfg}
echo "Create User Config file: ${cfg}"
echo " "

echo ""
echo "----------------------------------------------------------------"
echo "-- scanConsole with Local DB..."
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
