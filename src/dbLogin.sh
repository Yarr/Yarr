#!/bin/bash
#################################
# Contacts: Arisa Kubota
# Email: arisa.kubota at cern.ch
# Date: April 2019
# Project: Local Database for Yarr
# Description: Login Database 
# Usage: ./login_db.sh [-U <user account>*]
################################

DEBUG=false

# Usage
function usage {
    cat <<EOF

Usage:
    source /db_login.sh <user account>

EOF
}

function unset_variable {
    unset name
    unset cfg
    unset account
    unset institution
    unset identity
    unset answer
    unset dbnic
    unset cnt
    unset num
    unset DEV
    unset nic
    unset address
    unset dbcfg 
}

account=$1

if [ -z ${account} ]; then
    echo "Please give user account with '-U'."
    usage
    echo "Exit ..."
    unset_variable
    return 0
fi

if [ ! -e ${HOME}/.yarr ]; then
    mkdir ${HOME}/.yarr
fi

cfg=${HOME}/.yarr/${account}_user.json
if [ ! -f ${cfg} ]; then
    echo "User Config file is not exist: ${cfg}"
    echo " "
    echo "Enter your name (<first name>_<last name>) or 'exit' ... "
    read answer
    while [ -z ${answer} ]; 
    do
        echo "Enter your name (<first name>_<last name>) or 'exit' ... "
        read answer
    done
    if [ ${answer} == "exit" ]; then
        echo "Exit ..."
        unset_variable
        return 0
    else
        name=${answer}
    fi

    echo " "
    echo "Enter your institution (ABC_Laboratory) or 'exit' ... "
    read answer
    while [ -z ${answer} ]; 
    do
        echo "Enter your institution (ABC_Laboratory) or 'exit' ... "
        read answer
    done
    if [ ${answer} == "exit" ]; then
        echo "Exit ..."
        unset_variable
        return 0
    else
        institution=${answer}
    fi

    echo " "
    echo "Do you want to set identification key? [y/n]"
    read answer
    if [ "${answer}" == "n" ]; then
        identity="default"
    elif [ "${answer}" != "y" ]; then
        echo "Enter your identification key or 'cancel' ... "
        read answer
        while [ -z ${answer} ]; 
        do
            echo "Enter your identification key or 'cancel' ... "
            read answer
        done
        if [ ${answer} == "cancel" ]; then
            identity="default"
        else
            identity=${answer}
        fi
    fi
else
    echo "User Config file is exist: ${cfg}"
    name=`cat ${cfg}|grep 'userName'|awk -F'["]' '{print $4}'`
    institution=`cat ${cfg}|grep 'institution'|awk -F'["]' '{print $4}'`
    identity=`cat ${cfg}|grep 'userIdentity'|awk -F'["]' '{print $4}'`
fi

echo " "
echo "Logged in User Information"
echo "  Account: ${account}"
echo "  Name: ${name}"   
echo "  Institution: ${institution}"
echo "  Identity: ${identity}"
echo " "
echo "Are you sure that's correct? [y/n]"
read answer
while [ -z ${answer} ]; 
do
    echo "Are you sure that's correct? [y/n]"
    read answer
done

dbcfg=${HOME}/.yarr/database.json
if [ ! -f ${dbcfg} ]; then
    echo "{" > ${dbcfg}
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
    echo "        \"Flex + Bare Module Attachment\"" >> ${dbcfg}
    echo "    ]," >> ${dbcfg}
    echo "    \"environment\": [" >> ${dbcfg}
    echo "        \"lv\"," >> ${dbcfg}
    echo "        \"hv\"," >> ${dbcfg}
    echo "        \"lv_current\"," >> ${dbcfg}
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
fi
if [ ${answer} == "y" ]; then
    echo "{" > ${cfg}
    echo "    \"userName\": \"${name}\"," >> ${cfg}
    echo "    \"institution\": \"${institution}\"," >> ${cfg}
    echo "    \"userIdentity\": \"${identity}\"," >> ${cfg}
    echo "    \"dbCfg\": \"${dbcfg}\"" >> ${cfg}
    echo "}" >> ${cfg}
    echo " "
    if "${DEBUG}"; then
        echo "export DBUSER=\"${account}\""
        echo " "
    fi
    export DBUSER=${account}
    if "${DEBUG}"; then
        echo "./bin/dbAccessor -U ${account}"
        echo " "
    fi
    ./bin/dbAccessor -U ${account}
    echo "Create User Config file: ${cfg}"
    echo " "
else
    echo "Exit ..."
    echo " "
    unset_variable
    return 0
fi

declare -a nic=()  
num=0
for DEV in `find /sys/devices -name net | grep -v virtual`; 
do 
    nic[${num}]=`ls --color=none ${DEV}`
    num=$(( num + 1 ))
done
if [ ${num} != 1 ]; then
    echo "Select the number before NIC name for the information of this machine."
    echo " "
    cnt=0
    while [ ${cnt} -lt ${num} ]; do
        echo ${nic[0]}
        cnt=$(( cnt + 1 ))
    done
    read answer
    while [ -z ${answer} ]; 
    do
        echo "Select the number before NIC name for the information of this machine."
        echo " "
        read answer
    done
    echo ${answer} | grep [^0-9] > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo "Please give an integral as the number before NIC name."
        echo " "
        unset_variable
        return 0
    fi
    dbnic="${nic[${answer}]}"
else
    dbnic="${nic[0]}"
fi
address=${HOME}/.yarr/address
echo `ifconfig ${dbnic} | grep -o -E '([[:xdigit:]]{1,2}:){5}[[:xdigit:]]{1,2}'` > ${address}

if "${DEBUG}"; then
    echo "./bin/dbAccessor -S"
    echo " "
fi
./bin/dbAccessor -S

unset_variable

return 0
