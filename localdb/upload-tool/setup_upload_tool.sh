#!/bin/bash
#################################
# Author: Arisa Kubota
# Email: arisa.kubota at cern.ch
# Date: July 2019
# Project: Local Database for YARR
# Description: Setup upload tool for local use
#################################

ITSNAME="[LocalDB Tool Setup Retrieve Tool]"

# Start
echo -e "$ITSNAME Welcome!"

# Check python modules
echo -e "$ITSNAME Check python modules..."
cd ../
/usr/bin/env python3 check_python_modules.py || return
cd - >/dev/null

BIN=${HOME}/.local/bin
BASHLIB=${HOME}/.local/lib/localdb-tools/bash-completion
MODLIB=${HOME}/.local/lib/localdb-tools/modules
ENABLE=${HOME}/.local/lib/localdb-tools/enable

# Copy bin
chmod +x ./src/usr/local/bin/*
cp ./src/usr/local/bin/* ${BIN}/

# Copy library
mkdir -p ${BASHLIB}
mkdir -p ${MODLIB}
cp -r ./src/usr/local/lib/localdb-tools/bash-completion/* ${BASHLIB}/
cp -r ./src/usr/local/lib/localdb-tools/modules/* ${MODLIB}/

# Copy enable
echo "source ${BASHLIB}/completions/localdbtool-upload" >> ${ENABLE}

# Enable bash completion
source ${ENABLE}

echo -e "$ITSNAME Finish!"

echo -e "$ITSNAME Usage)"
echo -e "\tuse 'localdbtool-upload init' to initialize upload repository"
echo -e "\tand"
echo -e "\tuse 'localdbtool-upload remote add <remote name>' to add remote repository for Local DB/Master Server"
echo -e "\tthen you can upload data from Local DB/Master Server with some commands of 'localdb-tools'"
echo -e ""
echo -e "\tuse 'localdbtool-upload --help' to check more usage."
