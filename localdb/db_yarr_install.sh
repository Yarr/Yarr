#!/bin/bash
#################################
# Contacts: Arisa Kubota
# Email: arisa.kubota at cern.ch
# Date: April 2019
# Project: Local Database for Yarr
# Description: Login Database 
# Usage: ./db_yarr_install.sh
################################

DEBUG=false

# Usage
function usage {
    cat <<EOF

Usage:
    ./db_yarr_install.sh

EOF
}

### start installation

LOGFILE="instlog."`date "+%Y%m%d_%H%M%S"`
#exec 2>&1> >(awk '{print strftime("[%Y-%m-%d %H:%M:%S] "),$0 } { fflush() } ' | tee $LOGFILE)
exec 2> >(awk '{print strftime("[%Y-%m-%d %H:%M:%S] "),$0 } { fflush() } ' | tee $LOGFILE) 1>&2

trap 'echo ""; echo "Installation stopped by SIGINT!!"; echo "You may be in unknown state."; echo "Check ${LOGFILE} for debugging in case of a problem of re-executing this script."; exit 1' 2

#packages list to be required
yumpackages=(
    "epel-release.noarch"
    "centos-release-scl.noarch"
    "bc.x86_64"
    "rh-mongodb36-mongo-cxx-driver-devel.x86_64"
    "rh-mongodb36-boost-devel.x86_64"
    "mongodb-org.x86_64"
    "devtoolset-7.x86_64"
    "gnuplot.x86_64"
    "openssl-devel"
    "httpd.x86_64"
    "python.x86_64"
    "python36" 
    "python36-devel" 
    "python36-pip" 
    "python36-tkinter"
)
services=(
    "mongod"
)

#checking what is missing for localDB and viewer
echo "Looking for missing things for Yarr-localDB and its viewer..."
echo "-------------------------------------------------------------"
if [ ! -e "/etc/yum.repos.d/mongodb-org-3.6.repo" ]; then
    echo "Add: mongodb-org-3.6 repository in /etc/yum.repos.d/mongodb-org-3.6.repo."
fi
for pac in ${yumpackages[@]}; do
    if ! yum info ${pac} 2>&1 | grep "Installed Packages" > /dev/null; then
	echo "yum install: ${pac}"
    fi
done
for svc in ${services[@]}; do
    if ! systemctl status ${svc} 2>&1 | grep running > /dev/null; then
        echo "Start: ${svc}"
    fi
    if ! systemctl list-unit-files -t service|grep enabled 2>&1 | grep ${svc} > /dev/null; then
        echo "Enable: ${svc}"
    fi
done
echo "----------------------------------------------------"

#installing necessary packages if not yet installed
echo "Start installing necessary packages..."
#adding mongoDB repository and installing mongoDB
if [ -e "/etc/yum.repos.d/mongodb-org-3.6.repo" ]; then
    echo "mongodb-org-3.6 repository already installed. Nothing to do."
else
    echo "Adding mongodb-org-3.6 repository."
    sudo sh -c "echo \"[mongodb-org-3.6]
name=MongoDB Repository
baseurl=https://repo.mongodb.org/yum/redhat/7Server/mongodb-org/3.6/x86_64/
gpgcheck=1
enabled=1
gpgkey=https://www.mongodb.org/static/pgp/server-3.6.asc\" > /etc/yum.repos.d/mongodb-org-3.6.repo"
fi
#installing yum packages
for pac in ${yumpackages[@]}; do
    if yum info ${pac} | grep "Installed Packages" > /dev/null; then
        echo "${pac} already installed. Nothing to do."
    else
        echo "${pac} not found. Starting to install..."
        sudo yum install -y ${pac}
    fi
done

#enabling RedHad SCL packages
scl_sw=(
    "devtoolset-7"
    "rh-mongodb36"
)

#install python packages by pip for the DB viewer
cd ${setting_dir}
sudo pip3 install -r requirements.txt

echo ""
echo "Finished installation!!"
echo "Install log can be found in: $LOGFILE"
echo ""
echo "----------------------------------------------------------------"
echo "-- First thing to do..."
echo "----------------------------------------------------------------"
echo "Setup database config file by..." | tee -a README
echo "./setup_db.sh" | tee -a README
echo "" | tee -a README
echo "Scan with uploading Local DB by..." | tee README
echo "cd ../src" | tee -a README
echo "source /opt/rh/devtoolset-7/enable"
echo "source /opt/rh/rh-mongodb36/enable"
echo "make -j4" | tee -a README
echo "./bin/scanConsole -c configs/connectivity/example_rd53a_setup.json -r configs/controller/specCfg.json -s configs/scans/rd53a/std_digitalscan.json -W" | tee -a README
echo "./bin/dbAccessor -R ~/.yarr/localdb/var/cache/timestamp_directory" | tee -a README
echo "" | tee -a README
echo "Check results in the DB viewer in your web browser..." | tee -a README
echo "From the DAQ machine: http://localhost:5000/localdb/" | tee -a README
echo "From other machines : http://${ip}/localdb/" | tee -a README
echo "" | tee -a README
echo "Check more detail at..." | tee -a README
echo "" | tee -a README
echo "https://github.com/jlab-hep/Yarr/wiki/Quick-tutorial" | tee -a README
echo ""
echo "Prepared a README file for the reminder. Enjoy!!"

