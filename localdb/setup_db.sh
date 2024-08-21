#!/bin/bash
#################################
# Contacts: Arisa Kubota
# Email: arisa.kubota at cern.ch
# Date: July 2019
# Project: Local Database for Yarr
# Usage: ./setup_db.sh [-i Local DB server ip (default: 127.0.0.1)] [-p Local DB server port (default: 27017)] [-n Local DB name (default: localdb)] [-r Reset]
################################

#############
### Usage ###
#############

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

#############
### Start ###
#############

if [ `echo ${0} | grep bash` ]; then
    echo -e "[LDB] DO NOT 'source'"
    usage
    return
fi

shell_dir=$(cd $(dirname ${BASH_SOURCE}); pwd)
reset=false
while getopts hi:p:n:r OPT
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

#############
### Reset ###
#############

if "${reset}"; then
    echo -e "[LDB] Clean Local DB settings:"
    if [ -d ${BIN} ]; then
        for i in `ls -1 ${shell_dir}/bin/`; do
            if [ -f ${BIN}/${i} ]; then
                echo -e "[LDB]      -> remove ${BIN}/${i}"
            fi
        done
    fi
    if [ -d ${dir} ]; then
        for i in `ls -1 ${dir}`; do
            echo -e "[LDB]      -> remove ${dir}/${i}"
        done
    fi
    if [ -d ${HOME}/.localdb_retrieve ]; then
        for i in `ls -1 ${HOME}/.localdb_retrieve`; do
            echo -e "[LDB]      -> remove ${HOME}/.localdb_retrieve/${i}"
        done
    fi
    echo -e "[LDB] Continue? [y/n]"
    unset answer
    read -p "[LDB] > " answer
    while [ -z "${answer}" ];
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

####################
### Confirmation ###
####################

######################
### Check HOSTNAME ###
######################

if ! export -p | grep HOSTNAME > /dev/null 2>&1; then
    echo -e "[LDB] HOSTNAME environmental variable not found ... using default: default_host"
    export HOSTNAME="default_host"
fi

##########################
### Set editor command ###
##########################

echo -e "[LDB] Set editor command ... (e.g. nano, vim, emacs)"
unset answer
read -p "[LDB] > " answer
while [ -z "${answer}" ];
do
    read -p "[LDB] > " answer
done
EDITOR=${answer}
echo -e "[LDB]"

#############################
### Check python packages ###
#############################

echo -e "[LDB] Checking Python Packages ..."
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
        pac_mod=$( echo "${pac}" | sed -e "s/==.*//g" | sed -e "s/>=.*//g")
        if ! python3 -m pip list 2>&1 | grep ${pac_mod} 2>&1 > /dev/null; then
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
echo -e "[LDB]     ... OK!"
echo -e "[LDB]"

#################################
### Create Local DB Directory ###
#################################

mkdir -p ${dir}
mkdir -p ${dir}/log

##################################
### Check database config file ###
##################################

dbcfg=${dir}/${HOSTNAME}_database.json
dbcfg_exist=false
echo -e "[LDB] Checking Database Config: ${dbcfg} ..."
if [ -f ${dbcfg} ]; then
    printf '\033[33m%s\033[m\n' "[LDB WARNING] FOUND DATABASE CONFIG FILE: ${dbcfg}"
    dbcfg_dbip=`grep hostIp ${dbcfg} | awk '{print $2}' | tr -d ,\" | xargs echo`
    dbcfg_dbport=`grep hostPort ${dbcfg} | awk '{print $2}' | tr -d ,\" | xargs echo`
    dbcfg_dbname=`grep dbName ${dbcfg} | awk '{print $2}' | tr -d ,\" | xargs echo`
    if [ ! -z "${dbcfg_dbip}" -a ! -z "${dbcfg_dbport}" -a ! -z "${dbcfg_dbname}" ]; then
        dbcfg_exist=true
    else
        printf '\033[33m%s\033[m\n' "[LDB WARNING] !!! 'hostIp' OR 'hostPort' OR 'dbName' IS MISSING IN ${dbcfg} !!!"
        printf '\033[33m%s\033[m\n' "[LDB WARNING] !!! OVERRIDE BY DEFAULT FILE !!!"
        unset dbcfg_dbip
        unset dbcfg_dbport
        unset dbcfg_dbname
        echo -e "[LDB] Continue? [y/exit]"
        unset answer
        while [ -z "${answer}" ];
        do
            read -p "[LDB] > " answer
        done
        echo -e "[LDB]"
        if [[ ${answer} != "y" ]]; then
            echo -e "[LDB] Exit ..."
            echo -e "[LDB]"
            exit 0
        fi
    fi
fi
echo -e "[LDB]"
printf '\033[32m%s\033[m\n' "[LDB] -----------------------"
printf '\033[32m%s\033[m\n' "[LDB] --  Mongo DB Server  --"
printf '\033[32m%s\033[m\n' "[LDB] -----------------------"
if [ -z "${dbcfg_dbip}" ]; then
    if [ -z "${dbip}" ]; then
        dbip=127.0.0.1
    fi
    printf '\033[32m%s\033[m\n' "[LDB] IP address       : ${dbip}"
else
    if [ -z "${dbip}" ]; then
        dbip=${dbcfg_dbip}
    fi
    printf '\033[32m%s\033[m\n' "[LDB] IP address       : ${dbip} (current: ${dbcfg_dbip})"
fi
if [ -z "${dbcfg_dbport}" ]; then
    if [ -z "${dbport}" ]; then
        dbport=27017
    fi
    printf '\033[32m%s\033[m\n' "[LDB] port             : ${dbport}"
else
    if [ -z "${dbport}" ]; then
        dbport=${dbcfg_dbport}
    fi
    printf '\033[32m%s\033[m\n' "[LDB] port             : ${dbport} (current: ${dbcfg_dbport})"
fi
if [ -z "${dbcfg_dbname}" ]; then
    if [ -z "${dbname}" ]; then
        dbname="localdb"
    fi
    printf '\033[32m%s\033[m\n' "[LDB] database name    : ${dbname}"
else
    if [ -z "${dbname}" ]; then
        dbname=${dbcfg_dbname}
    fi
    printf '\033[32m%s\033[m\n' "[LDB] database name    : ${dbname} (current: ${dbcfg_dbname})"
fi
printf '\033[32m%s\033[m\n' "[LDB] -----------------------"
echo -e "[LDB]"
echo -e "[LDB] Are you sure that is correct? (Move to edit mode when answer 'n') [y/n/exit]"
unset answer
read -p "[LDB] > " answer
while [ -z "${answer}" ];
do
    read -p "[LDB] > " answer
done
if [[ ${answer} = "exit" ]]; then
    echo -e "[LDB] Exit ..."
    echo -e "[LDB]"
    exit 0
fi
if "${dbcfg_exist}"; then
    sed -i -e "s!`grep hostIp ${dbcfg} |  tr -d ,`!    \"hostIp\": \"${dbip}\"!g" ${dbcfg}
    sed -i -e "s!`grep hostPort ${dbcfg} |  tr -d ,`!    \"hostPort\": \"${dbport}\"!g" ${dbcfg}
    sed -i -e "s!`grep dbName ${dbcfg} |  tr -d ,`!    \"dbName\": \"${dbname}\"!g" ${dbcfg}
else
    cp ${shell_dir}/setting/default/database.json ${dbcfg}
    sed -i -e "s!DBIP!${dbip}!g" ${dbcfg}
    sed -i -e "s!DBPORT!${dbport}!g" ${dbcfg}
    sed -i -e "s!DBNAME!${dbname}!g" ${dbcfg}
fi
if [[ ${answer} != "y" ]]; then
    ${EDITOR} ${dbcfg}
fi
echo -e "[LDB] Created Database Config: ${dbcfg}."
echo -e "[LDB]"

##############################
### Check user config file ###
##############################

usercfg=${dir}/user.json
usercfg_exist=false
echo -e "[LDB] Checking User Config: ${usercfg} ..."
if [ -f ${usercfg} ]; then
    printf '\033[33m%s\033[m\n' "[LDB WARNING] FOUND USER CONFIG FILE: ${usercfg}"
    usercfg_username=`grep userName ${usercfg} | awk '{$1=""; print}' | tr -d ,\" | xargs echo`
    usercfg_institution=`grep institution ${usercfg} | awk '{$1=""; print}' | tr -d ,\" | xargs echo`
    if [ ! -z "${usercfg_username}" -a ! -z "${usercfg_institution}" ]; then
        usercfg_exist=true
    else
        printf '\033[33m%s\033[m\n' "[LDB WARNING] !!! 'userName' OR 'institution' IS MISSING IN ${usercfg} !!!"
        printf '\033[33m%s\033[m\n' "[LDB WARNING] !!! OVERRIDE BY DEFAULT FILE !!!"
        unset usercfg_username
        unset usercfg_institution
        echo -e "[LDB] Continue? [y/exit]"
        unset answer
        while [ -z "${answer}" ];
        do
            read -p "[LDB] > " answer
        done
        echo -e "[LDB]"
        if [[ ${answer} != "y" ]]; then
            echo -e "[LDB] Exit ..."
            echo -e "[LDB]"
            exit 0
        fi
    fi
fi
echo -e "[LDB]"
printf '\033[32m%s\033[m\n' "[LDB] -----------------------"
printf '\033[32m%s\033[m\n' "[LDB] --  User Information --"
printf '\033[32m%s\033[m\n' "[LDB] -----------------------"
if [ -z "${usercfg_username}" ]; then
    if [ -z "${username}" ]; then
        username=${USER}
    fi
    printf '\033[32m%s\033[m\n' "[LDB] User Name        : ${username}"
else
    if [ -z "${username}" ]; then
        username=${usercfg_username}
    fi
    printf '\033[32m%s\033[m\n' "[LDB] User Name        : ${username} (current: ${usercfg_username})"
fi
if [ -z "${usercfg_institution}" ]; then
    if [ -z "${institution}" ]; then
        institution=${HOSTNAME}
    fi
    printf '\033[32m%s\033[m\n' "[LDB] User Institution : ${institution}"
else
    if [ -z "${institution}" ]; then
        institution=${usercfg_institution}
    fi
    printf '\033[32m%s\033[m\n' "[LDB] User Institution : ${institution} (current: ${usercfg_institution})"
fi
printf '\033[32m%s\033[m\n' "[LDB] -----------------------"
echo -e "[LDB]"
echo -e "[LDB] Are you sure that is correct? (Move to edit mode when answer 'n') [y/n/exit]"
unset answer
read -p "[LDB] > " answer
while [ -z "${answer}" ];
do
    read -p "[LDB] > " answer
done
if [[ ${answer} = "exit" ]]; then
    echo -e "[LDB] Exit ..."
    echo -e "[LDB]"
    exit 0
fi
if "${usercfg_exist}"; then
    sed -i -e "s!`grep userName ${usercfg} |  tr -d ,`!    \"userName\": \"${username}\"!g" ${usercfg}
    sed -i -e "s!`grep institution ${usercfg} |  tr -d ,`!    \"institution\": \"${institution}\"!g" ${usercfg}
else
    cp ${shell_dir}/setting/default/user.json ${usercfg}
    sed -i -e "s!NAME!${username}!g" ${usercfg}
    sed -i -e "s!INSTITUTION!${institution}!g" ${usercfg}
fi
if [[ ${answer} != "y" ]]; then
    ${EDITOR} ${usercfg}
fi
echo -e "[LDB] Created User Config: ${usercfg}"
echo -e "[LDB]"

##############################
### Check site config file ###
##############################

sitecfg=${dir}/${HOSTNAME}_site.json
sitecfg_exist=false
echo -e "[LDB] Checking Site Config: ${sitecfg} ..."
if [ -f ${sitecfg} ]; then
    printf '\033[33m%s\033[m\n' "[LDB WARNING] FOUND SITE CONFIG FILE: ${sitecfg}"
    sitecfg_sitename=`grep institution ${sitecfg} | awk '{$1=""; print}' | tr -d ,\" | xargs echo`
    if [ ! -z "${sitecfg_sitename}" ]; then
        sitecfg_exist=true
    else
        printf '\033[33m%s\033[m\n' "[LDB WARNING] !!! 'institution' IS MISSING IN ${sitecfg} !!!"
        printf '\033[33m%s\033[m\n' "[LDB WARNING] !!! OVERRIDE BY DEFAULT FILE !!!"
        unset sitecfg_sitename
        echo -e "[LDB] Continue? [y/exit]"
        unset answer
        while [ -z "${answer}" ];
        do
            read -p "[LDB] > " answer
        done
        echo -e "[LDB]"
        if [[ ${answer} != "y" ]]; then
            echo -e "[LDB] Exit ..."
            echo -e "[LDB]"
            exit 0
        fi
    fi
fi
echo -e "[LDB]"
printf '\033[32m%s\033[m\n' "[LDB] -----------------------"
printf '\033[32m%s\033[m\n' "[LDB] --  Site Information --"
printf '\033[32m%s\033[m\n' "[LDB] -----------------------"
if [ -z "${sitecfg_sitename}" ]; then
    if [ -z "${sitename}" ]; then
        sitename=${HOSTNAME}
    fi
    printf '\033[32m%s\033[m\n' "[LDB] site name        : ${sitename}"
else
    if [ -z "${sitename}" ]; then
        sitename=${sitecfg_sitename}
    fi
    printf '\033[32m%s\033[m\n' "[LDB] site name        : ${sitename} (current: ${sitecfg_sitename})"
fi
printf '\033[32m%s\033[m\n' "[LDB] -----------------------"
echo -e "[LDB]"
echo -e "[LDB] Are you sure that is correct? (Move to edit mode when answer 'n') [y/n/exit]"
unset answer
read -p "[LDB] > " answer
while [ -z "${answer}" ];
do
    read -p "[LDB] > " answer
done
if [[ ${answer} = "exit" ]]; then
    echo -e "[LDB] Exit ..."
    echo -e "[LDB]"
    exit 0
fi
if "${sitecfg_exist}"; then
    sed -i -e "s!`grep institution ${sitecfg} |  tr -d ,`!    \"institution\": \"${sitename}\"!g" ${sitecfg}
else
    cp ${shell_dir}/setting/default/site.json ${sitecfg}
    sed -i -e "s!SITE!${sitename}!g" ${sitecfg}
fi
if [[ ${answer} != "y" ]]; then
    ${EDITOR} ${sitecfg}
fi
echo -e "[LDB] Created Site Config: ${sitecfg}."
echo -e "[LDB]"

##########################
### Final Confirmation ###
##########################

echo -e "[LDB] Checking the connection..."
${shell_dir}/bin/localdbtool-upload init --config ${dbcfg}
if [ $? = 1 ]; then
    printf '\033[33m%s\033[m\n' "[LDB WARNING] Check Local DB Setting in ${dbcfg}."
fi

echo -e "[LDB] Done."
echo -e "[LDB]"

# finish
echo -e "[LDB] To upoad the test data into Local DB after scanConsole:"
echo -e "[LDB]   \$ ./bin/scanConsole -c <conn> -r <ctr> -s <scan> -W"
echo -e "[LDB] More detail:"
echo -e "[LDB]   Access 'https://localdb-docs.readthedocs.io/en/master/' (master branch)"
echo -e "[LDB]   Access 'https://localdb-docs.readthedocs.io/en/devel/'  (devel branch)"
