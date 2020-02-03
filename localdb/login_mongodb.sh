#!/bin/bash
##################################################################
# Log in mongoDB
# Usage : ./login_mongodb.sh [-i IP address] [-p port]
# Contacts : Hiroki Okuyama (okuyama@hep.phys.titech.ac.jp)
##################################################################
ITSNAME="[LDB]"

if [ ! `echo ${0} | grep bash` ]; then
    echo -e "Use 'source'"
    exit
fi

# Usage
function usage {
    cat <<EOF

Usage:
    ./setup_viewer.sh [-i ip address] [-p port] [-c config]

Options:
    - i <IP address>  Local DB server IP address, default: 127.0.0.1
    - p <port>        Local DB server port, default: 27017
    - c <config>      Config file name, default: conf.yml

EOF
}

### Confirmation
which mongo > /dev/null 2>&1
if [ $? = 1 ]; then
    printf '\033[31m%s\033[m\n' "[ERROR] 'mongo' command is required."
    exit 1
fi

tools_dir=$(cd $(dirname $0); pwd)

dbip=127.0.0.1
dbport=27017

while getopts i:p:c: OPT
do
    case ${OPT} in
        i ) dbip=${OPTARG} ;;
        p ) dbport=${OPTARG} ;;
        u ) user_config=${OPTARG} ;;
        a ) admin_config=${OPTARG} ;;
        * ) usage
            exit ;;
    esac
done

echo "Local DB Server IP address: ${dbip}"
echo "Local DB Server port: ${dbport}"
echo " "
echo "${ITSNAME} Are you sure that's correct? [y/n]"
unset answer
read -p "> " answer
while [ -z ${answer} ];
do
echo "${ITSNAME} Are you sure that's correct? [y/n]"
    read -p "> " answer
done
echo " "
if [ ${answer} != "y" ]; then
    printf '\033[31m%s\033[m\n' "[ERROR] Try again login_mongodb.sh, Exit ..."
    exit 1
fi

# input username and password
read -p "Input mongoDB username: " user
if [ -z ${user} ]; then
    printf '\033[31m%s\033[m\n' "[ERROR] Does not match the administrator account."
    printf '\033[31m%s\033[m\n' "        Please input correct password. Try again login_mongodb.sh, Exit ..."
    exit 1
fi

read -sp "Input mongoDB password: " password
if [ -z ${password} ]; then
    printf '\033[31m%s\033[m\n' "[ERROR] Does not match the administrator account."
    printf '\033[31m%s\033[m\n' "        Please input correct password. Try again login_mongodb.sh, Exit ..."
    exit 1
fi

# confirmation 
python3 << EOF
import sys
from pymongo import MongoClient, errors
url = 'mongodb://${dbip}:${dbport}'
client = MongoClient(url,serverSelectionTimeoutMS=1)
localdb = client['localdb']
try:
    localdb.authenticate('${user}','${password}')
    sys.exit(0)
except errors.OperationFailure as err:
    print(err)
    sys.exit(1)
EOF
if [ $? = 1 ]; then
    echo -e ""
    printf '\033[31m%s\033[m\n' "[ERROR] You cannot connect this LocalDB because protected."
    printf '\033[31m%s\033[m\n' "        Please input correct username and password."
    exit 1
else
    echo -e "" 
    printf "${ITSNAME} Authentication succeeded!"
fi

export username=${user}
export password=${password}

echo ""
echo "${ITSNAME} You can use LocalDB tools."

