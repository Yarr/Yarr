#!/bin/bash
#################################
# Contacts: Arisa Kubota
# Email: arisa.kubota at cern.ch
# Date: July 2020
# Project: Local Database for Yarr
# Usage: source login_mongodb.sh [-d path/to/db/config]
################################

function usage {
    cat <<EOF

Login Local DB with your account by:

    source login_mongodb.sh

You can specify the DB config as following:

    source login_mongodb.sh [-d path/to/db/config]

    - h         Show this usage
    - d <path>  path to DB config file (default: ${HOME}/.yarr/localdb/${HOSTNAME}_database.json)
    - Q         set QC scan mode

EOF
}

function unsetvariables {
    unset md5command
    unset username_hash
    unset password_hash
    unset dbcfg
    unset usercfg
    unset sitecfg
    unset qc
}

if [ ! `echo ${0} | grep bash` ]; then
    echo -e "Use 'source'"
    exit
fi

qc=false
dbcfg=${HOME}/.yarr/localdb/${HOSTNAME}_database.json
OPTIND=1
while getopts hd:Q OPT
do
    case ${OPT} in
        h ) usage
            exit ;;
        d ) dbcfg=${OPTARG} ;;
        Q ) qc=true ;;
        * ) usage
            exit ;;
    esac
done

shell_dir=$(cd $(dirname ${BASH_SOURCE}); pwd)
md5command=md5sum
which ${md5command} > /dev/null 2>&1
if [ $? = 1 ]; then
    md5command=md5
    which md5 >/dev/null 2>&1
    if [ $? = 1 ]; then
        printf '\033[31m%s\033[m\n' "[ERROR] 'md5sum' or 'md5' command is required."
        unsetvariables
        return 1
    fi
fi

read -p "Input mongodb account's username: " username
read -sp "Input mongodb account's password: " password
echo ""
username_hash=`echo -n ${username}|${md5command}|sed -e "s/-//"|sed -e "s/ //g"`
password_hash=`echo -n ${password}|${md5command}|sed -e "s/-//"|sed -e "s/ //g"`
export username=${username}
export password=${password_hash}

usercfg=${HOME}/.yarr/localdb/user.json
sitecfg=${HOME}/.yarr/localdb/${HOSTNAME}_site.json

python3 << EOF
import json
path = '${usercfg}'
doc = { 'viewerUser': '${username}' }
with open(path, 'w') as f:
    json.dump(doc, f, indent=4)
EOF

if "${qc}"; then
${shell_dir}/bin/localdbtool-retrieve user --user ${usercfg} --site ${sitecfg} --QC
else
${shell_dir}/bin/localdbtool-retrieve user --user ${usercfg} --site ${sitecfg}
fi
if [ $? -ne 0 ]; then
    echo ""
    printf '\033[31m%s\033[m\n' "[LDB] Login failed."
    unsetvariables
    unset username
    unset password
    return 1
fi
echo ""
echo "[LDB] Login successful."
echo "[LDB] Username and password are saved."
unsetvariables
