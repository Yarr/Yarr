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
    ./db_login.sh [user account*] 

Options:
    - user account* required.

EOF
}

account=$1

if [ -z ${account} ]; then
    echo "Please give user account with '-U'."
    usage
    echo "Exit ..."
    exit 
fi

if [ ! -e ${HOME}/.yarr ]; then
    mkdir ${HOME}/.yarr
fi

cfg=${HOME}/.yarr/${account}_user.json
if [ ! -f ${cfg} ]; then
    echo "User Config file is not exist: ${cfg}"
    echo " "
    echo "Enter your name (<first name> <last name>) or 'exit' ... "
    read -p "> " -a answer
    while [ ${#answer[@]} == 0 ]; 
    do
        echo "Enter your name (<first name> <last name>) or 'exit' ... "
        read -p "> " -a answer
    done
    if [ ${answer[0]} == "exit" ]; then
        echo "Exit ..."
        exit
    else
        for a in ${answer[@]}; do
            name="${name#_}_${a}"
        done
    fi

    echo " "
    echo "Enter your institution (ABC Laboratory) or 'exit' ... "
    read -p "> " -a answer
    while [ ${#answer[@]} == 0 ]; 
    do
        echo "Enter your institution (ABC Laboratory) or 'exit' ... "
        read -p "> " -a answer
    done
    if [ ${answer[0]} == "exit" ]; then
        echo "Exit ..."
        exit
    else
        for a in ${answer[@]}; do
            institution="${institution#_}_${a}"
        done
    fi

    echo " "
    echo "You can set the identification keyword if you want. (e.x. nickname, SW version you use ...)"
    echo "Do you want to set the identification keyword? [y/n]"
    echo "    y => continue to set the identification keyword"
    echo "    n => set the identification keyword 'default'"
    read -p "> " -a answer
    while [ ${#answer[@]} == 0 ]; 
    do
        echo "Enter 'y' to set the identification keyword, 'n' not to set, or 'exit' to abort ... "
        read -p "> " -a answer
    done
    if [ ${answer[0]} == "exit" ]; then
        echo "Exit ..."
        exit
    fi
    if [ ${answer[0]} != "y" ]; then
        identity="default"
    else
        echo " "
        echo "Enter your identification keyword or 'cancel' ... "
        read -p "> " -a answer
        while [ ${#answer[@]} == 0 ]; 
        do
            echo "Enter your identification keyword or 'cancel' ... "
            read -p "> " -a answer
        done
        if [ ${answer[0]} == "cancel" ]; then
            identity="default"
        else
            for a in ${answer[@]}; do
                identity="${identity#_}_${a}"
            done
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
read -p "> " answer
while [ -z ${answer} ]; 
do
    echo "Are you sure that's correct? [y/n]"
    read -p "> " answer
done
echo " "

if [ ${answer} != "y" ]; then
    echo "Exit ..."
    echo " "
    exit
fi

echo "{" > ${cfg}
echo "    \"userName\": \"${name}\"," >> ${cfg}
echo "    \"institution\": \"${institution}\"," >> ${cfg}
echo "    \"userIdentity\": \"${identity}\"" >> ${cfg}
echo "}" >> ${cfg}
echo "Create User Config file: ${cfg}"
echo " "

#echo "Register user and site data"
#if "${DEBUG}"; then
#    echo "./bin/dbAccessor -U ${account}"
#    echo " "
#fi
#./bin/dbAccessor -U ${account}

exit
