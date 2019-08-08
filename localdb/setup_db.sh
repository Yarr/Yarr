#!/bin/bash
#################################
# Contacts: Arisa Kubota
# Email: arisa.kubota at cern.ch
# Date: July 2019
# Project: Local Database for Yarr
# Usage: ./setup_db.sh [-i Local DB server ip (default: 127.0.0.1)] [-p Local DB server port (default: 27017)] [-n Local DB name (default: localdb)] [-t set tools in ~/.local/bin ] [-r Reset]
################################

set -e

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
    - a <CA file>     Path to CA certificate
    - e <Certificate> Path to Client certificate
    - t               Set LocalDB tools into ${HOME}/.local/bin
    - r               Clean the settings (reset)

EOF
}

# Start
if [ `echo ${0} | grep bash` ]; then
    echo -e "[LDB] DO NOT 'source'"
    usage
    return
fi

shell_dir=$(cd $(dirname ${BASH_SOURCE}); pwd)
tools=false
reset=false
dbca=null
dbcert=null
while getopts hi:p:n:a:e:tr OPT
do
    case ${OPT} in
        h ) usage 
            exit ;;
        i ) dbip=${OPTARG} ;;
        p ) dbport=${OPTARG} ;;
        n ) dbname=${OPTARG} ;;
        a ) dbca=${OPTARG} ;;
        e ) dbcert=${OPTARG} ;;
        t ) tools=true ;;
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
    echo -e "[LDB]      -> remove Local DB Tools in ${BIN}"
    for i in `ls -1 ${shell_dir}/bin/`; do
        if [ -f ${BIN}/${i} ]; then
            echo -e "[LDB]         - ${i}"
        fi
    done
    echo -e "[LDB]      -> remove Local DB files in ${dir}"
    echo -e "[LDB]      -> remove retrieve repository in ${HOME}/.localdb_retrieve"
    echo -e "[LDB] Continue? [y/n]"
    read -p "[LDB] > " answer
    while [ -z ${answer} ]; 
    do
        read -p "[LDB] > " answer
    done
    echo -e "[LDB]"
    if [[ ${answer} != "y" ]]; then
        echo "[LDB] Exit ..."
        echo "[LDB]"
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

# Confirmation
echo -e "[LDB] Confirmation"
echo -e "[LDB] -----------------------"
echo -e "[LDB] --  Mongo DB Server  --"
echo -e "[LDB] -----------------------"
echo -e "[LDB] IP address      : ${dbip}"
echo -e "[LDB] port            : ${dbport}"
echo -e "[LDB] database name   : ${dbname}"
echo -e "[LDB] CA file         : ${dbca}"
echo -e "[LDB] Certificate file: ${dbcert}"
echo -e "[LDB]"
echo -e "[LDB] Are you sure that is correct? [y/n]"
read -p "[LDB] > " answer
while [ -z ${answer} ]; 
do
    read -p "[LDB] > " answer
done
echo -e "[LDB]"
if [[ ${answer} != "y" ]]; then
    echo "[LDB] Exit ..."
    echo "[LDB]"
    exit
fi

echo -e "[LDB] This script performs ..."
echo -e "[LDB]"
echo -e "[LDB]  - Check pip packages: '${shell_dir}/setting/requirements-pip.txt'"
if [ ! -d ${dir} ]; then
    echo -e "[LDB]  - Create Local DB Directory: ${dir}"
fi
if [ ! -d ${dir}/log ]; then
    echo -e "[LDB]  - Create Local DB Log Directory: ${dir}/log"
fi
echo -e "[LDB]  - Create Local DB files: ${dir}/${HOSTNAME}_database.json"
if [ -f ${dir}/${HOSTNAME}_database.json ]; then
    echo -e "[WARNING] !!! OVERRIDE THE EXISTING FILE: ${dir}/${HOSTNAME}_database.json !!!"
fi
if "${tools}"; then
    echo -e "[LDB]  - Set Local DB Tools:"
    for i in `ls -1 ${shell_dir}/bin/`; do
        echo -e "[LDB]     - ${BIN}/${i}"
    done
    echo -e "[LDB]     - ${HOME}/.local/lib/localdb-tools/"
fi
echo -e "[LDB]  - Create README: ${shell_dir}/README"
echo -e "[LDB]"
echo -e "[LDB] Continue? [y/n]"
unset answer
while [ -z ${answer} ]; 
do
    read -p "[LDB] > " answer
done
echo -e "[LDB]"
if [[ ${answer} != "y" ]]; then
    echo -e "[LDB] Exit ..."
    echo -e "[LDB]"
    exit
fi

# Check python module
echo -e "[LDB] Check python modules..."
if ! which python3 > /dev/null 2>&1; then
    echo -e "[LDB ERROR] Not found the command: python3"
    exit
fi
/usr/bin/env python3 ${shell_dir}/setting/check_python_version.py
if [ $? = 1 ]; then
    echo -e "[LDB ERROR] Python version 3.4 or later is required."
    exit
fi
/usr/bin/env python3 ${shell_dir}/setting/check_python_modules.py
if [ $? = 1 ]; then
    echo -e "[LDB ERROR] There are missing pip modules."
    echo -e "[LDB ERROR] Install them by:"
    echo -e "[LDB ERROR] pip3 install --user -r ${shell_dir}/requirements-pip.txt"
    exit
fi

# Create localdb directory
mkdir -p ${dir}
mkdir -p ${dir}/log

# Create database config
echo -e "[LDB] Create Config file..."
dbcfg=${dir}/${HOSTNAME}_database.json
cp ${shell_dir}/setting/default/database.json ${dbcfg}
sed -i -e "s!DBIP!${dbip}!g" ${dbcfg}
sed -i -e "s!DBPORT!${dbport}!g" ${dbcfg}
sed -i -e "s!DBNAME!${dbname}!g" ${dbcfg}
sed -i -e "s!CAFILE!\"${dbca}\"!g" ${dbcfg}
sed -i -e "s!PEMKEYFILE!\"${dbcert}\"!g" ${dbcfg}
if [ ${dbca} != "null" ] || [ ${dbcert} != "null" ]; then
    sed -i -e "s!ENABLED!true!g" ${dbcfg}
    sed -i -e "s!MECHANISM!MONGODB-X509!g" ${dbcfg}
else
    sed -i -e "s!ENABLED!false!g" ${dbcfg}
    sed -i -e "s!MECHANISM!SCRAM-SHA-256!g" ${dbcfg}
fi
    
echo -e "[LDB] DB Config: ${dbcfg}"

echo -e "[LDB] Done."
echo -e "[LDB]"

readme=${shell_dir}/README

if [ -f ${readme} ]; then
    rm ${readme}
fi

echo -e "# scanConsole with Local DB" > ${readme}
echo -e "" > ${readme}

if "${tools}"; then
    # Setting function
    mkdir -p ${BIN}
    mkdir -p ${BASHLIB}
    mkdir -p ${MODLIB}
    
    cp ${shell_dir}/bin/* ${BIN}/
    chmod +x ${BIN}/*
    
    cp -r ${shell_dir}/lib/localdb-tools/bash-completion/completions/* ${BASHLIB}/
    cp -r ${shell_dir}/lib/localdb-tools/modules/* ${MODLIB}/
    cp ${shell_dir}/lib/localdb-tools/enable ${ENABLE}

    if ! cat $HOME/.bashrc | grep 'export PATH="$HOME/.local/bin:$PATH"' 2>&1 > /dev/null; then
        # Add PATH on ~/.local/bin in bashrc
        echo -e "[LDB] Add PATH on ~/.local/bin"
        echo 'export PATH="$HOME/.local/bin:$PATH"' >> $HOME/.bashrc
        echo -e "[LDB]"
    fi
fi

# settings
echo -e "## Settings" > ${readme}
echo -e "- './setup_db.sh'" > ${readme}
echo -e "  - description: setup Local DB functions for the user local." > ${readme}
echo -e "  - requirements: required softwares" > ${readme}
echo -e "" > ${readme}

# upload.py
ITSNAME="LocalDB Tool Setup Upload Tool"
echo -e "### $ITSNAME" > ${readme}
echo -e "- '${shell_dir}/bin/localdbtool-upload scan <path to result directory>' can upload scan data" > ${readme}
echo -e "- '${shell_dir}/bin/localdbtool-upload dcs <path to result directory>' can upload dcs data based on scan data" > ${readme}
echo -e "- '${shell_dir}/bin/localdbtool-upload cache' can upload every cache data" > ${readme}
echo -e "- '${shell_dir}/bin/localdbtool-upload --help' can show more usage." > ${readme}
echo -e "" > ${readme}

# retrieve.py
ITSNAME="LocalDB Tool Setup Retrieve Tool"
echo -e "### $ITSNAME" > ${readme}
echo -e "- '${shell_dir}/bin/localdbtool-retrieve init' can initialize retrieve repository" > ${readme}
echo -e "- '${shell_dir}/bin/localdbtool-retrieve remote add <remote name>' can add remote repository for Local DB/Master Server" > ${readme}
echo -e "- '${shell_dir}/bin/localdbtool-retrieve --help' can show more usage." > ${readme}
echo -e "" > ${readme}

# finish
ITSNAME="Usage"
echo -e "## $ITSNAME" > ${readme}
echo -e "1. scanConsole with Local DB" > ${readme}
echo -e "   - './bin/scanConsole -c <conn> -r <ctr> -s <scan> -W' can use Local DB schemes" > ${readme}
echo -e "2. Upload function" > ${readme}
echo -e "   - '${shell_dir}/bin/localdbtool-upload cache' can upload every cache data" > ${readme}
echo -e "3. Retrieve function" > ${readme}
echo -e "   - '${shell_dir}/bin/localdbtool-retrieve log' can show test data log in Local DB" > ${readme}
echo -e "   - '${shell_dir}/bin/localdbtool-retrieve checkout <module name>' can restore the latest config files from Local DB" > ${readme}
echo -e "4. Viewer Application" > ${readme}
echo -e "   - Access 'http://HOSTNAME/localdb/' can display results in web browser if Viewer is running" > ${readme}
echo -e "   - (HOSTNAME: Local DB Server where web browser if Viewer is running" > ${readme}
echo -e "5. More Detail" > ${readme}
echo -e "   - Check 'https://github.com/jlab-hep/Yarr/wiki'" > ${readme}
echo -e "[LDB] The detail is written in ${readme}"
