#!/bin/bash
#################################
# Contacts: Arisa Kubota
# Email: arisa.kubota at cern.ch
# Date: July 2019
# Project: Local Database for Yarr
# Usage: ./setup_db.sh [-i Local DB server ip (default: 127.0.0.1)] [-p Local DB server port (default: 27017)] [-n Local DB name (default: localdb)] [-t set tools in ~/.local/bin ] [-r Reset]
################################

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
    echo -e "[LDB] DO NOT 'source'"
    usage
    return
fi

shell_dir=$(cd $(dirname ${BASH_SOURCE}); pwd)
reset=false
while getopts hi:p:n:a:e:tr OPT
do
    case ${OPT} in
        h ) usage
            exit ;;
        i ) dbip=${OPTARG} ;;
        p ) dbport=${OPTARG} ;;
        n ) dbname=${OPTARG} ;;
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
    if [ -d ${BIN} ]; then
        for i in `ls -1 ${shell_dir}/bin/`; do
            if [ -f ${BIN}/${i} ]; then
                echo -e "[LDB]      -> remove ${BIN}/${i}"
            fi
        done
    fi
    echo -e "[LDB]      -> remove Local DB files in ${dir}"
    if [ -d ${HOME}/.localdb_retrieve ]; then
        echo -e "[LDB]      -> remove retrieve repository in ${HOME}/.localdb_retrieve"
    fi
    echo -e "[LDB] Continue? [y/n]"
    unset answer
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
    bin_empty=true
    if [ -d ${BIN} ]; then
        for i in `ls -1 ${shell_dir}/bin/`; do
            if [ -f ${BIN}/${i} ]; then
                rm ${BIN}/${i}
            fi
        done
        for i in `ls -1 ${BIN}`; do
            if [ `echo ${i} | grep localdbtool` ]; then
                bin_empty=false
            fi
        done
    fi
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
    printf '\033[33m%s\033[m\n' "[LDB WARNING] !!! OVERRIDE THE EXISTING FILE: ${dir}/${HOSTNAME}_database.json !!!"
fi
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

pytver=false
if which python3 > /dev/null 2>&1; then
    python3 <<EOF
import sys
if not sys.version_info[0]==3: sys.exit(1)
if not sys.version_info[1]>=4: sys.exit(1)
sys.exit(0)
EOF
    if [ $? = 0 ]; then
        pytver=true
    fi
fi
if ! "${pytver}"; then
    printf '\033[31m%s\033[m\n' "[LDB ERROR] Python version 3.4 or later is required."
    exit 1
fi

pippackages=$(cat ${shell_dir}/setting/requirements-pip.txt)
if "${pytver}"; then
    for pac in ${pippackages[@]}; do
        if ! python3 -m pip list 2>&1 | grep ${pac} 2>&1 > /dev/null; then
            piparray+=(${pac})
        fi
    done
    pippackages=${piparray[@]}
fi
if [ ${#pippackages} != 0 ]; then
    printf '\033[31m%s\033[m\n' "[LDB ERROR] There are missing pip modules:"
    for pac in ${pippackages[@]}; do
        printf '\033[31m%s\033[m\n' "[LDB ERROR] - ${pac}"
    done
    printf '\033[31m%s\033[m\n' "[LDB ERROR]"
    printf '\033[31m%s\033[m\n' "[LDB ERROR] Install them by:"
    printf '\033[31m%s\033[m\n' "[LDB ERROR] python3 -m pip install --user -r ${shell_dir}/setting/requirements-pip.txt"
    exit 1
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

echo -e "[LDB] DB Config: ${dbcfg}"

# Confirmation
echo -e ""
echo -e "[LDB] Checking the connection..."
${shell_dir}/bin/localdbtool-upload init --config ${dbcfg}
if [ $? = 1 ]; then
    printf '\033[33m%s\033[m\n' "[LDB WARNING] Connection failed. Check Local DB Setting in ${dbcfg}."
fi

echo -e "[LDB] Done."
echo -e "[LDB]"


# finish
echo -e "[LDB] -------------"
echo -e "[LDB] --  Usage  --"
echo -e "[LDB] -------------"
echo -e "[LDB] To upoad the test data into Local DB after scanConsole:"
echo -e "[LDB]   \$ ./bin/scanConsole -c <conn> -r <ctr> -s <scan> -W"
echo -e "[LDB] To upload every cache data:"
echo -e "[LDB]   \$ ${shell_dir}/bin/localdbtool-upload cache"
echo -e "[LDB] To display the test data log from Local DB:"
echo -e "[LDB]   \$ ${shell_dir}/bin/localdbtool-retrieve log"
echo -e "[LDB] To retrieve the latest data files from Local DB:"
echo -e "[LDB]   \$ ${shell_dir}/bin/localdbtool-retrieve pull"
echo -e "[LDB] More detail:"
echo -e "[LDB]   Access 'https://localdb-docs.readthedocs.io/en/master/'"
