#!/bin/bash
#################################
# Contacts: Arisa Kubota
# Email: arisa.kubota at cern.ch
# Date: July 2019
# Project: Local Database for Yarr
# Description: Login Database 
# Usage: ./db_yarr_install.sh
################################

set -e

# Usage
function usage {
    cat <<EOF

Usage:
    ./db_yarr_install.sh

EOF
}

# Start
if [ `echo ${0} | grep bash` ]; then
    echo -e "[LDB] DO NOT 'source'"
    usage
    return
fi
shell_dir=$(cd $(dirname ${BASH_SOURCE}); pwd)
ip=`hostname -i`
yumpackages=$(cat ${shell_dir}/setting/requirements-yum.txt)
pippackages=$(cat ${shell_dir}/setting/requirements-pip.txt)
LOGDIR="${shell_dir}/setting/instlog"
if [ ! -d ${LOGDIR} ]; then
    mkdir ${LOGDIR}
fi
services=(
    "mongod"
)
# Confirmation
echo -e "[LDB] This script performs ..."
echo -e ""
echo -e "[LDB]  - Install yum packages: '${shell_dir}/setting/requirements-yum.txt'"
echo -e "[LDB]         $ sudo yum install \$(cat ${shell_dir}/setting/requirements-yum.txt)"
echo -e "[LDB]  - Install pip modules: '${shell_dir}/setting/requirements-pip.txt'"
echo -e "[LDB]         $ sudo pip3 install \$(cat ${shell_dir}/setting/requirements-pip.txt)"
echo -e "[LDB] Continue? [y/n]"
while [ -z ${answer} ]; 
do
    read -p "> " answer
done
echo -e ""
if [ ${answer} != "y" ]; then
    echo -e "[LDB] Exit..."
    echo -e ""
    echo -e "[LDB] If you want to setup them manually, the page 'https://github.com/jlab-hep/Yarr/wiki/Installation' should be helpful!"
    echo -e ""
    exit
fi
sudo echo -e "[LDB] OK!"

# Set log file
LOGFILE="${LOGDIR}/`date "+%Y%m%d_%H%M%S"`"
exec 2> >(awk '{print strftime("[%Y-%m-%d %H:%M:%S] "),$0 } { fflush() } ' | tee ${LOGFILE}) 1>&2
trap 'echo -e ""; echo -e "[LDB] Installation stopped by SIGINT!!"; echo -e "[LDB] You may be in unknown state."; echo -e "[LDB] Check ${LOGFILE} for debugging in case of a problem of re-executing this script."; exit 1' 2

# Check what is missing for Local DB
echo -e "[LDB] Looking for missing things for Local DB and its Tools..."
echo -e "[LDB] -------------------------------------------------------------"
if [ ! -e "/etc/yum.repos.d/mongodb-org-3.6.repo" ]; then
    echo -e "[LDB] Add: mongodb-org-3.6 repository in /etc/yum.repos.d/mongodb-org-3.6.repo."
fi
for pac in ${yumpackages[@]}; do
    if ! yum list installed 2>&1 | grep ${pac} > /dev/null; then
	echo -e "[LDB] yum install: ${pac}"
    fi
done
for pac in ${pippackages[@]}; do
    if ! pip3 list 2>&1 | grep ${pac} 2>&1 > /dev/null; then
       echo -e "[LDB] pip3 install: ${pac}"
    fi
done
for svc in ${services[@]}; do
    if ! systemctl status ${svc} 2>&1 | grep running > /dev/null; then
        echo -e "[LDB] Start: ${svc}"
    fi
    if ! systemctl list-unit-files -t service|grep enabled 2>&1 | grep ${svc} > /dev/null; then
        echo -e "[LDB] Enable: ${svc}"
    fi
done
echo -e "[LDB] ----------------------------------------------------"

# Install necessary packages if not yet installed
echo -e "[LDB] Start installing necessary packages..."
# Add mongoDB repository and installing mongoDB
if [ -e "/etc/yum.repos.d/mongodb-org-3.6.repo" ]; then
    echo -e "[LDB] mongodb-org-3.6 repository already installed. Nothing to do."
else
    echo -e "[LDB] Adding mongodb-org-3.6 repository."
    sudo sh -c "echo \"[mongodb-org-3.6]
name=MongoDB Repository
baseurl=https://repo.mongodb.org/yum/redhat/7Server/mongodb-org/3.6/x86_64/
gpgcheck=1
enabled=1
gpgkey=https://www.mongodb.org/static/pgp/server-3.6.asc\" > /etc/yum.repos.d/mongodb-org-3.6.repo"
fi
# Install yum packages
for pac in ${yumpackages[@]}; do
    if yum list installed 2>&1 | grep ${pac} > /dev/null; then
        echo -e "[LDB] ${pac} already installed. Nothing to do."
    else
        echo -e "[LDB] ${pac} not found. Starting to install..."
        sudo yum install -y ${pac}
    fi
done

# Enable RedHad SCL packages
source /opt/rh/devtoolset-7/enable

# Install python packages by pip for the DB viewer
for pac in ${pippackages[@]}; do
    if pip3 list 2>&1 | grep ${pac} 2>&1 > /dev/null; then
        echo "${pac} already installed. Nothing to do."
    else
        echo "${pac} not found. Starting to install..."
        sudo pip3 install ${pac}
    fi
done
/usr/bin/env python3 ${shell_dir}/check_python_modules.py
if [ $? = 1 ]; then
    echo -e "[LDB] Failed, exit..."
    exit
fi
echo -e "[LDB] Done."
echo -e ""

readme=${shell_dir}/setting/README.md

echo -e ""
echo -e "Finished installation!!"
echo -e "Install log can be found in: ${LOGFILE}"
echo -e ""
echo -e "# Local DB Installation for DAQ Server" | tee ${readme}
echo -e "" | tee -a ${readme}
echo -e "## 1. Setup database config and function" | tee -a ${readme}
echo -e "\`\`\`" | tee -a ${readme}
echo -e "./setup_db.sh" | tee -a ${readme}
echo -e "source /opt/rh/devtoolset-7/enable" | tee ${readme}
echo -e "source ${HOME}/.local/lib/localdb-tools/enable" | tee ${readme}
echo -e "\`\`\`" | tee -a ${readme}
echo -e "" | tee -a ${readme}
echo -e "## 2. Setup scanConsole" | tee ${readme}
echo -e "\`\`\`" | tee -a ${readme}
echo -e "cd YARR" | tee -a ${readme}
echo -e "mkdir build" | tee -a ${readme}
echo -e "cmake3 ../" | tee -a ${readme}
echo -e "make -j4" | tee -a ${readme}
echo -e "make install" | tee -a ${readme}
echo -e "\`\`\`" | tee -a ${readme}
echo -e "" | tee -a ${readme}
echo -e "## 3. Scan with Local DB" | tee ${readme}
echo -e "\`\`\`" | tee -a ${readme}
echo -e "./bin/scanConsole -c configs/connectivity/example_rd53a_setup.json -r configs/controller/specCfg.json -s configs/scans/rd53a/std_digitalscan.json -W" | tee -a ${readme}
echo -e "\`\`\`" | tee -a ${readme}
echo -e "" | tee -a ${readme}
echo -e "## 4. Check results in the DB viewer in your web browser" | tee -a ${readme}
echo -e "- From the DB machine: http://localhost:5000/localdb/" | tee -a ${readme}
echo -e "- From other machines : http://${ip}/localdb/" | tee -a ${readme}
echo -e "" | tee -a ${readme}
echo -e "## 5.Check more detail" | tee -a ${readme}
echo -e "- https://github.com/jlab-hep/Yarr/wiki" | tee -a ${readme}
echo -e "This description is saved as ${readme}. Enjoy!!"
