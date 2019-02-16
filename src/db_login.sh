#!/bin/bash
#################################
# Contacts: Arisa Kubota (akubota@hep.phys.titech.ac.jp)
# Project: Yarr
# Description: Login Database 
# Usage: ./login_db.sh [-U <user account>*]
# Date: Mar 2019
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
}

account=$1

if [ -z ${account} ]; then
    echo "Please give user account with '-U'."
    usage
    echo "Exit ..."
    unset_variable
    return 0
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

if [ ${answer} == "y" ]; then
    echo "{" > ${cfg}
    echo "    \"userName\": \"${name}\"," >> ${cfg}
    echo "    \"institution\": \"${institution}\"," >> ${cfg}
    echo "    \"userIdentity\": \"${identity}\"" >> ${cfg}
    echo "}" >> ${cfg}
    echo " "
    if "${DEBUG}"; then
        echo "export DBUSER=\"${account}\""
    fi
    export DBUSER=${account}
    if "${DEBUG}"; then
        echo "./bin/dbRegister -U ${account}"
    fi
    ./bin/dbRegister -U ${account}
    echo " "
    echo "Create User Config file: ${cfg}"
else
    echo "Exit ..."
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
    cnt=0
    while [ ${cnt} -lt ${num} ]; do
        echo ${nic[0]}
        cnt=$(( cnt + 1 ))
    done
    read answer
    while [ -z ${answer} ]; 
    do
        echo "Select the number before NIC name for the information of this machine."
        read answer
    done
    echo ${answer} | grep [^0-9] > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo "Please give an integral as the number before NIC name."
        return 0
    fi
    dbnic="${nic[${answer}]}"
else
    dbnic="${nic[0]}"
fi
address=${HOME}/.yarr/address
echo `ifconfig ${dbnic} | grep -o -E '([[:xdigit:]]{1,2}:){5}[[:xdigit:]]{1,2}'` > ${address}

unset_variable

return 0
