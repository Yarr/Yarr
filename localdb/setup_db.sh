#!/bin/bash
#################################
# Contacts: Arisa Kubota
# Email: arisa.kubota at cern.ch
# Date: July 2019
# Project: Local Database for Yarr
# Usage: ./setup_db.sh [-i Local DB server ip (default: 127.0.0.1)] [-p Local DB server port (default: 27017)] [-n Local DB name (default: localdb)] [-r Reset]
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
echo -e "[LDB] IP address: ${dbip}"
echo -e "[LDB] port: ${dbport}"
echo -e "[LDB] CA file: ${dbca}"
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
echo -e "[LDB]  - Create Local DB files:"
echo -e "[LDB]         ---> ${dir}"
echo -e "[LDB]         ---> ${dir}/log"
echo -e "[LDB]         ---> ${dir}/${HOSTNAME}_database.json"
if "${tools}"; then
    echo -e "[LDB]  - Set Local DB Tools:"
    for i in `ls -1 ${shell_dir}/bin/`; do
        if [ -f ${BIN}/${i} ]; then
            echo -e "[LDB]         ---> ${BIN}/${i}"
        fi
    done
    echo -e "[LDB]         ---> ${HOME}/.local/lib/localdb-tools/"
fi
echo -e "[LDB]  - Create README:"
echo -e "[LDB]         ---> ${shell_dir}/README"
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
    echo -e "[LDB] You can set the Local DB tools by:"
    echo -e "[LDB]     $ ./setup_db.sh -t"
    echo -e "[LDB] If you want to setup them manually, the page 'https://github.com/jlab-hep/Yarr/wiki/Installation' should be helpful!"
    echo -e ""
    exit
fi

# Check python module
echo -e "[LDB] Check python modules..."
/usr/bin/env python3 ${shell_dir}/setting/check_python_modules.py
if [ $? = 1 ]; then
    echo -e "[LDB] Failed!!!"
    echo -e "[LDB] Install the missing modules by:"
    echo -e "[LDB] pip3 install --user -r ${shell_dir}/requirements-pip.txt"
    echo -e "[LDB] exit..."
    exit
fi

# Create localdb directory
mkdir -p ${dir}
mkdir -p ${dir}/log

# Create database config
echo -e "[LDB] Create Config files..."
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
echo -e ""

readme=${shell_dir}/README

if [ -f ${readme} ]; then
    rm ${readme}
fi

echo -e "# scanConsole with Local DB" | tee -a ${readme}
echo -e "" | tee -a ${readme}

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
echo -e "[LDB] This description is saved as ${readme}"
