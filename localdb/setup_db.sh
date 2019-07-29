#!/bin/bash
#################################
# Contacts: Arisa Kubota
# Email: arisa.kubota at cern.ch
# Date: July 2019
# Project: Local Database for Yarr
# Usage: ./setup_db.sh [-i Local DB server ip (default: 127.0.0.1)] [-p Local DB server port (default: 27017)] [-n Local DB name (default: localdb)] [-r Reset]
################################

set -e

DEBUG=false

# Usage
function usage {
    cat <<EOF

Make some config files and Set some tools for Local DB in ${HOME}/.yarr/localdb by:

    ./setup_db.sh

You can specify the IP address, port number, and DB name of Local DB as following:

    ./setup_db.sh [-i ip address] [-p port] [-n db name]

    - h               Show this usage
    - i <ip address>  Local DB server ip address (default: 127.0.0.1)
    - p <port>        Local DB server port (default: 27017)
    - n <db name>     Local DB Name (default: localdb)
    - r               Clean the settings (reset)

EOF
}

# Start
if [ `echo ${0} | grep bash` ]; then
    echo "DO NOT 'source'"
    usage
    return
fi

shell_dir=$(cd $(dirname ${BASH_SOURCE}); pwd)
setting_dir=${shell_dir}/setting
reset=false
while getopts i:p:n:hr OPT
do
    case ${OPT} in
        i ) dbip=${OPTARG} ;;
        p ) dbport=${OPTARG} ;;
        n ) dbname=${OPTARG} ;;
        h ) usage 
            exit ;;
        r ) reset=true ;;
        * ) usage
            exit ;;
    esac
done

dir=${HOME}/.yarr/localdb
BIN=${HOME}/.local/bin
BASHLIB=${HOME}/.local/lib/localdb-tools/bash-completion/completions
MODLIB=${HOME}/.local/lib/localdb-tools/modules
ENABLE=${HOME}/.local/lib/localdb-tools/enable

if "${reset}"; then
    echo -e "[LDB] Clean Local DB settings:"
    echo -e "      -> remove Local DB Tools in ${BIN}"
    for i in `ls -1 ${shell_dir}/bin/`; do
        if [ -f ${BIN}/${i} ]; then
            echo -e "         ${i}"
        fi
    done
    echo -e "      -> remove Local DB files in ${dir}"
    echo -e "Continue? [y/n]"
    read -p "> " answer
    while [ -z ${answer} ]; 
    do
        read -p "> " answer
    done
    echo -e ""
    if [[ ${answer} != "y" ]]; then
        echo "[LDB] Exit ..."
        echo " "
        exit
    fi
    # binary
    for i in `ls -1 ${shell_dir}/bin/`; do
        if [ -f ${BIN}/${i} ]; then
            rm ${BIN}/${i}
        fi
    done
    bin_empty=true
    for i in `ls -1 ${BIN}`; do
        if [ `echo ${i} | grep localdbtool` ]; then
            bin_empty=false
        fi
    done
    if "${bin_empty}"; then
        if [ -d ${HOME}/.local/lib/localdb-tools ]; then
            rm -r ${HOME}/.local/lib/localdb-tools
        fi
    fi
    # library
    if ! "${bin_empty}"; then
        for i in `ls -1 ${shell_dir}/lib/localdb-tools/bash-completion/completions/`; do
            if [ -f ${BASHLIB}/${i} ]; then
                rm ${BASHLIB}/${i}
            fi
        done
        for i in `ls -1 ${shell_dir}/lib/localdb-tools/modules/`; do
            if [ -f ${MODLIB}/${i} ]; then
                rm ${MODLIB}/${i}
            fi
        done
        lib_empty=true
        for i in `ls -1 ${MODLIB}`; do
            if [ `echo ${i} | grep py` ]; then
                lib_empty=false
            fi
        done
        for i in `ls -1 ${BASHLIB}`; do
            if [ `echo ${i} | grep localdbtool` ]; then
                lib_empty=false
            fi
        done
        if [ ${lib_empty} ]; then
            if [ -d ${HOME}/.local/lib/localdb-tools ]; then
                rm -r ${HOME}/.local/lib/localdb-tools
            fi
        fi
    fi
    if [ -d ${dir} ]; then
        rm -r ${dir}
    fi
    if [ -d ${HOME}/.localdb_retrieve ]; then
        rm -r ${HOME}/.localdb_retrieve
    fi
    echo -e "[LDB] Finish Clean Up!"
    exit
fi

if [ -z ${dbip} ]; then
    dbip=127.0.0.1
fi
if [ -z ${dbport} ]; then
    dbport=27017
fi
if [ -z ${dbname} ]; then
    dbname="localdb"
fi

# Check the name of site
site=${dir}/site.json
declare -a nic=()  
num=0
for DEV in `find /sys/devices -name net | grep -v virtual`; 
do 
    nic[${num}]=`ls --color=none ${DEV}`
    num=$(( num + 1 ))
done
dbnic="${nic[0]}"
if [ -f ${site} ]; then
    tmpmacaddress=`cat ${site}|grep 'macAddress'|awk -F'["]' '{print $4}'`
    tmpname=`cat ${site}|grep 'name'|awk -F'["]' '{print $4}'`
    tmpinstitution=`cat ${site}|grep 'institution'|awk -F'["]' '{print $4}'`
fi
macaddress=`ifconfig ${dbnic} | grep -o -E '([[:xdigit:]]{1,2}:){5}[[:xdigit:]]{1,2}'`
if [ -f ${site} ] && [ ${macaddress} = ${tmpmacaddress} ] && [ ${HOSTNAME} = ${tmpname} ] && [ -n ${tmpinstitution} ]; then
    echo -e "[LDB] Check the exist site config...OK! ${site}"
    echo -e ""
    institution=${tmpinstitution}
else
    echo -e "[LDB] Check the exist site config...NG!"
    echo -e ""
    echo -e "[LDB] Enter the institution name where this machine is, or 'exit' ... "
    read -p "> " -a answer
    while [ ${#answer[@]} == 0 ]; 
    do
        read -p "> " -a answer
    done
    echo -e ""
    if [[ ${answer[0]} == "exit" ]]; then
        echo -e "[LDB] Exit ..."
        echo -e ""
        exit
    else
        for a in ${answer[@]}; do
            institution="${institution#_}_${a}"
        done
        institution="${institution#_}"
    fi
fi

# Confirmation
echo -e "[LDB] Confirmation"
echo -e "\t-----------------------"
echo -e "\t--  Mongo DB Server  --"
echo -e "\t-----------------------"
echo -e "\tIP address: ${dbip}"
echo -e "\tport: ${dbport}"
echo -e "\t-----------------"
echo -e "\t--  Test Site  --"
echo -e "\t-----------------"
echo -e "\tMAC address: ${macaddress}"
echo -e "\tMachine Name: ${HOSTNAME}"   
echo -e "\tInstitution: ${institution}"
echo -e ""
echo -e "[LDB] Are you sure that's correct? [y/n]"
read -p "> " answer
while [ -z ${answer} ]; 
do
    read -p "> " answer
done
echo -e ""
if [[ ${answer} != "y" ]]; then
    if [ -f ${site} ]; then 
        rm ${site}
    fi
    echo "[LDB] Exit ..."
    echo " "
    exit
fi

# Create localdb directory
mkdir -p ${dir}
mkdir -p ${dir}/log
# Create database config
echo -e "[LDB] Create Config files..."
dbcfg=${dir}/database.json
cp ${setting_dir}/default/database.json ${dbcfg}
sed -i -e "s!DBIP!${dbip}!g" ${dbcfg}
sed -i -e "s!DBPORT!${dbport}!g" ${dbcfg}
sed -i -e "s!DBNAME!${dbname}!g" ${dbcfg}
echo -e "\tDB Config: ${dbcfg}"

# Create site site config 
cp ${setting_dir}/default/site.json ${site}
sed -i -e "s!MACADDR!${macaddress}!g" ${site}
sed -i -e "s!HOSTNAME!${HOSTNAME}!g" ${site}
sed -i -e "s!SITE!${institution}!g" ${site}
echo -e "\tSite Config: ${site}"

# Create default user config
cfg=${dir}/user.json
cp ${setting_dir}/default/user.json ${cfg}
sed -i -e "s!NAME!${USER}!g" ${cfg}
sed -i -e "s!INSTITUTION!${HOSTNAME}!g" ${cfg}
echo -e "\tUser Config: ${cfg}"
echo -e ""

# Add PATH on ~/.local/bin in bashrc
echo -e "\tAdd PATH on ~/.local/bin"
echo 'export PATH="$HOME/.local/bin:$PATH"' >> $HOME/.bashrc
echo -e ""

# Check python module
echo -e "[LDB] Check python modules..."
pip3 install --user -r ${setting_dir}/requirements-pip.txt 2&> /dev/null && :
/usr/bin/env python3 ${setting_dir}/check_python_modules.py
if [ $? = 1 ]; then
    echo -e "[LDB] Failed, exit..."
    exit
fi
echo -e "[LDB] Done."
echo -e ""

readme=${shell_dir}/README

if [ -f ${readme} ]; then
    rm ${readme}
fi

echo -e "# scanConsole with Local DB" | tee -a ${readme}
echo -e "" | tee -a ${readme}

# Setting function
mkdir -p ${BIN}
mkdir -p ${BASHLIB}
mkdir -p ${MODLIB}

cp ${shell_dir}/bin/* ${BIN}/
chmod +x ${BIN}/*

cp -r ${shell_dir}/lib/localdb-tools/bash-completion/completions/* ${BASHLIB}/
cp -r ${shell_dir}/lib/localdb-tools/modules/* ${MODLIB}/
cp ${shell_dir}/lib/localdb-tools/enable ${ENABLE}

# settings
echo -e "## Settings" | tee -a ${readme}
echo -e "- 'Makefile'" | tee -a ${readme}
echo -e "  - description: install required softwares and setup Local DB functions for the machine." | tee -a ${readme}
echo -e "  - requirements: sudo user, git, net-tools" | tee -a ${readme}
echo -e "- './setup_db.sh'" | tee -a ${readme}
echo -e "  - description: setup Local DB functions for the user local." | tee -a ${readme}
echo -e "  - requirements: required softwares" | tee -a ${readme}
echo -e "" | tee -a ${readme}

# all function
ITSNAME="LocalDB Tools"
echo -e "## $ITSNAME" | tee -a ${readme}
echo -e "'source ${HOME}/.local/lib/localdb-tools/enable' can enable tab-completion" | tee -a ${readme}
echo -e "" | tee -a ${readme}

# upload.py
ITSNAME="LocalDB Tool Setup Upload Tool"
echo -e "### $ITSNAME" | tee -a ${readme}
echo -e "- 'localdbtool-upload --scan <path to result directory>' can upload scan data" | tee -a ${readme}
echo -e "- 'localdbtool-upload --dcs <path to result directory>' can upload dcs data based on scan data" | tee -a ${readme}
echo -e "- 'localdbtool-upload --cache' can upload every cache data" | tee -a ${readme}
echo -e "- 'localdbtool-upload --help' can show more usage." | tee -a ${readme}
echo -e "" | tee -a ${readme}

# retrieve.py
ITSNAME="LocalDB Tool Setup Retrieve Tool"
echo -e "### $ITSNAME" | tee -a ${readme}
echo -e "- 'localdbtool-retrieve init' can initialize retrieve repository" | tee -a ${readme}
echo -e "- 'localdbtool-retrieve remote add <remote name>' can add remote repository for Local DB/Master Server" | tee -a ${readme}
echo -e "- 'localdbtool-retrieve --help' can show more usage." | tee -a ${readme}
echo -e "" | tee -a ${readme}

# finish
ITSNAME="Usage"
echo -e "## $ITSNAME" | tee -a ${readme}
echo -e "1. scanConsole with Local DB" | tee -a ${readme}
echo -e "   - './bin/scanConsole -c <conn> -r <ctr> -s <scan> -W' can use Local DB schemes" | tee -a ${readme}
echo -e "2. Upload function" | tee -a ${readme}
echo -e "   - 'localdbtool-upload --cache' can upload every cache data" | tee -a ${readme}
echo -e "3. Retrieve function" | tee -a ${readme}
echo -e "   - 'localdbtool-retrieve log' can show test data log in Local DB" | tee -a ${readme}
echo -e "   - 'localdbtool-retrieve checkout <module name>' can restore the latest config files from Local DB" | tee -a ${readme}
echo -e "4. Viewer Application" | tee -a ${readme}
echo -e "   - Access 'http://HOSTNAME/localdb/' can display results in web browser if Viewer is running" | tee -a ${readme}
echo -e "   - (HOSTNAME: Local DB Server where web browser if Viewer is running" | tee -a ${readme}
echo -e "5. More Detail" | tee -a ${readme}
echo -e "   - Check 'https://github.com/jlab-hep/Yarr/wiki'" | tee -a ${readme}
echo -e "" | tee -a ${readme}
echo -e "This description is saved as ${readme}"
