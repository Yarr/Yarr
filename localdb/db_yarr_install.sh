#!/bin/bash
#################################
# Contacts: Arisa Kubota
# Email: arisa.kubota at cern.ch
# Date: April 2019
# Project: Local Database for Yarr
# Description: Login Database 
# Usage: ./db_yarr_install.sh
################################

set -e

DEBUG=false

# Usage
function usage {
    cat <<EOF

Usage:
    ./db_yarr_install.sh

EOF
}

### start installation
sudo echo "[LDB] OK!"
setting_dir=$(cd $(dirname ${BASH_SOURCE}); pwd)
LOGDIR="${setting_dir}/setting/instlog"
if [ ! -d ${LOGDIR} ]; then
    mkdir ${LOGDIR}
fi
LOGFILE="${LOGDIR}/`date "+%Y%m%d_%H%M%S"`"
exec 2> >(awk '{print strftime("[%Y-%m-%d %H:%M:%S] "),$0 } { fflush() } ' | tee $LOGFILE) 1>&2

trap 'echo -e ""; echo -e "[LDB] Installation stopped by SIGINT!!"; echo -e "[LDB] You may be in unknown state."; echo -e "[LDB] Check ${LOGFILE} for debugging in case of a problem of re-executing this script."; exit 1' 2

#packages list to be required
yumpackages=(
    "epel-release.noarch"
    "centos-release-scl.noarch"
    "bc.x86_64"
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
echo -e "[LDB] Looking for missing things for Yarr-localDB and its viewer..."
echo -e "[LDB] -------------------------------------------------------------"
if [ ! -e "/etc/yum.repos.d/mongodb-org-3.6.repo" ]; then
    echo -e "[LDB] Add: mongodb-org-3.6 repository in /etc/yum.repos.d/mongodb-org-3.6.repo."
fi
for pac in ${yumpackages[@]}; do
    if ! yum info ${pac} 2>&1 | grep "Installed Packages" > /dev/null; then
	echo -e "[LDB] yum install: ${pac}"
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

#installing necessary packages if not yet installed
echo -e "[LDB] Start installing necessary packages..."
#adding mongoDB repository and installing mongoDB
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
#installing yum packages
for pac in ${yumpackages[@]}; do
    if yum info ${pac} | grep "Installed Packages" > /dev/null; then
        echo -e "[LDB] ${pac} already installed. Nothing to do."
    else
        echo -e "[LDB] ${pac} not found. Starting to install..."
        sudo yum install -y ${pac}
    fi
done

#enabling RedHad SCL packages
scl_sw=(
    "devtoolset-7"
)

#install python packages by pip for the DB viewer
cd ${setting_dir}/setting
sudo pip3 install -r requirements.txt
/usr/bin/env python3 ${setting_dir}/check_python_modules.py
if [ $? = 1 ]; then
    echo -e "[LDB] Failed, exit..."
    exit
fi
echo -e "[LDB] Done."
echo -e ""
cd -

readme=${setting_dir}/setting/README.md

if [ -f ${readme} ]; then
    rm ${readme}
fi

echo -e ""
echo -e "Finished installation!!"
echo -e "Install log can be found in: ${LOGFILE}"
echo -e ""
echo -e "# Local DB Installation for DAQ Server" | tee -a ${readme}
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
echo -e "- From the DAQ machine: http://localhost:5000/localdb/" | tee -a ${readme}
echo -e "- From other machines : http://${ip}/localdb/" | tee -a ${readme}
echo -e "" | tee -a ${readme}
echo -e "## 5.Check more detail" | tee -a ${readme}
echo -e "- https://github.com/jlab-hep/Yarr/wiki/Quick-tutorial" | tee -a ${readme}
echo -e "This description is saved as ${readme}. Enjoy!!"

