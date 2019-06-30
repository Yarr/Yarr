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
dbcfg=${dir}/etc/${HOSTNAME}_database.json
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
address=${dir}/etc/${HOSTNAME}_address.json
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

### start installation

LOGFILE="instlog."`date "+%Y%m%d_%H%M%S"`
#exec 2>&1> >(awk '{print strftime("[%Y-%m-%d %H:%M:%S] "),$0 } { fflush() } ' | tee $LOGFILE)
exec 2> >(awk '{print strftime("[%Y-%m-%d %H:%M:%S] "),$0 } { fflush() } ' | tee $LOGFILE) 1>&2

trap 'echo ""; echo "Installation stopped by SIGINT!!"; echo "You may be in unknown state."; echo "Check ${LOGFILE} for debugging in case of a problem of re-executing this script."; exit 1' 2

#packages list to be required
yumpackages=(
    "epel-release.noarch"
    "centos-release-scl.noarch"
    "bc.x86_64"
    "wget.x86_64"
    "rh-mongodb36-mongo-cxx-driver-devel.x86_64"
    "rh-mongodb36-boost-devel.x86_64"
    "mongodb-org.x86_64"
    "devtoolset-7.x86_64"
    "gnuplot.x86_64"
)
services=(
    "mongod"
)

#checking what is missing for localDB and viewer
echo "Looking for missing things for Yarr-localDB and its viewer..."
echo "-------------------------------------------------------------"
if [ ! -e "/etc/yum.repos.d/mongodb-org-3.6.repo" ]; then
    echo "Add: mongodb-org-3.6 repository in /etc/yum.repos.d/mongodb-org-3.6.repo."
fi
for pac in ${yumpackages[@]}; do
    if ! yum info ${pac} 2>&1 | grep "Installed Packages" > /dev/null; then
	echo "yum install: ${pac}"
    fi
done
if ! getsebool httpd_can_network_connect | grep off > /dev/null; then
    echo "SELinux: turning on httpd_can_network_connect"
fi
if ! sudo firewall-cmd --list-all | grep http > /dev/null; then
    echo "Firewall: opening port=80/tcp for appache."
fi
if ! sudo firewall-cmd --list-ports --zone=public --permanent | grep 5000/tcp > /dev/null; then
    echo "Firewall: opening port=5000/tcp for viewer application."
fi
for svc in ${services[@]}; do
    if ! systemctl status ${svc} 2>&1 | grep running > /dev/null; then
        echo "Start: ${svc}"
    fi
    if ! systemctl list-unit-files -t service|grep enabled 2>&1 | grep ${svc} > /dev/null; then
        echo "Enable: ${svc}"
    fi
done
echo "----------------------------------------------------"

#installing necessary packages if not yet installed
echo "Start installing necessary packages..."
#adding mongoDB repository and installing mongoDB
if [ -e "/etc/yum.repos.d/mongodb-org-3.6.repo" ]; then
    echo "mongodb-org-3.6 repository already installed. Nothing to do."
else
    echo "Adding mongodb-org-3.6 repository."
    sudo sh -c "echo \"[mongodb-org-3.6]
name=MongoDB Repository
baseurl=https://repo.mongodb.org/yum/redhat/7Server/mongodb-org/3.6/x86_64/
gpgcheck=1
enabled=1
gpgkey=https://www.mongodb.org/static/pgp/server-3.6.asc\" > /etc/yum.repos.d/mongodb-org-3.6.repo"
fi
#installing yum packages
for pac in ${yumpackages[@]}; do
    if yum info ${pac} | grep "Installed Packages" > /dev/null; then
        echo "${pac} already installed. Nothing to do."
    else
        echo "${pac} not found. Starting to install..."
        sudo yum install -y ${pac}
    fi
done

#enabling RedHad SCL packages
scl_sw=(
    "devtoolset-7"
    "rh-mongodb36"
)
for sw in ${scl_sw[@]}; do
    echo "Checking if ${sw} is already enabled in .bashrc..."
    if grep "source /opt/rh/${sw}/enable" ~/.bashrc > /dev/null; then
        echo "Already setup. Nothing to do."
    else
        echo "Not found. Adding a source command in your .bashrc"
        echo -e "\n#added by the mongoDB install script" >> ~/.bashrc
        echo "source /opt/rh/${sw}/enable" >> ~/.bashrc
    fi
    source /opt/rh/${sw}/enable
done

echo ""
echo "Finished installation!!"
echo "Install log can be found in: $LOGFILE"
echo ""
echo "----------------------------------------------------------------"
echo "-- First thing to do..."
echo "----------------------------------------------------------------"
echo "Scan with uploading Local DB by..." | tee README
echo "cd ../src" | tee -a README
echo "./bin/scanConsole -c configs/connectivity/example_rd53a_setup.json -r configs/controller/specCfg.json -s configs/scans/rd53a/std_digitalscan.json -W" | tee -a README
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

