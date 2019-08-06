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


while getopts h OPT
do
    case ${OPT} in
        h ) usage 
            exit ;;
        * ) usage
            exit ;;
    esac
done


shell_dir=$(cd $(dirname ${BASH_SOURCE}); pwd)
ip=`hostname -i`
echo -e "[LDB] Looking for missing things for Local DB and its Tools..."

### check python
if which python3 > /dev/null 2>&1; then
    python_exist=true
    /usr/bin/env python3 ${shell_dir}/setting/check_python_version.py
    if [ $? = 1 ]; then
        echo -e "[LDB WARNING] Python version 3.4 or later is required."
        python_version=false
    else
        python_version=true
        ### check python packages
        pippackages=$(cat ${shell_dir}/setting/requirements-pip.txt)
        for pac in ${pippackages[@]}; do
            if ! pip3 list 2>&1 | grep ${pac} 2>&1 > /dev/null; then
                piparray+=(${pac})
            fi
        done
        pippackages=${piparray[@]}
    fi
else
    echo -e "[LDB WARNING] Not found the command: python3"
    python_exist=false
fi

### If not python3.4 or later
if ! "${python_version}" || ! "${python_exist}"; then
    ### check yum packages
    yumpackages=$(cat ${shell_dir}/setting/requirements-yum.txt)
    for pac in ${yumpackages[@]}; do
        if ! yum list installed 2>&1 | grep ${pac} > /dev/null; then
            yumarray+=(${pac})
        fi
    done
    yumpackages=${yumarray[@]}
fi

echo -e "[LDB] Done."

### Confirmation
if [ ${#yumpackages} != 0 ] || [ ${#pippackages} != 0 ]; then
    ### Installation
    if [ ${#yumpackages} != 0 ]; then
        echo -e "[LDB WARNING] --------------------"
        echo -e "[LDB WARNING] Missing yum packages"
        echo -e "[LDB WARNING] --------------------"
        for pac in ${yumpackages[@]}; do
            echo -e "[LDB WARNING] ${pac}"
        done
    fi
    if [ ${#pippackages} != 0 ]; then
        echo -e "[LDB WARNING] --------------------"
        echo -e "[LDB WARNING] Missing pip packages"
        echo -e "[LDB WARNING] --------------------"
        for pac in ${pippackages[@]}; do
            echo -e "[LDB WARNING] ${pac}"
        done
    fi
    echo -e "[LDB WARNING] --------------------"
    echo -e "[LDB] Install these package? [y/n]"
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
    ## Set log file
    LOGDIR="${shell_dir}/setting/instlog"
    if [ ! -d ${LOGDIR} ]; then
        mkdir ${LOGDIR}
    fi
    LOGFILE="${LOGDIR}/`date "+%Y%m%d_%H%M%S"`"
    exec 2> >(awk '{print strftime("[%Y-%m-%d %H:%M:%S] "),$0 } { fflush() } ' | tee ${LOGFILE}) 1>&2
    trap 'echo -e ""; echo -e "[LDB] Installation stopped by SIGINT!!"; echo -e "[LDB] You may be in unknown state."; echo -e "[LDB] Check ${LOGFILE} for debugging in case of a problem of re-executing this script."; exit 1' 2
    
    ### Install necessary packages if not yet installed
    echo -e "[LDB] Start installing necessary packages..."
    
    ### Install yum packages
    for pac in ${yumpackages[@]}; do
        echo -e "[LDB] ${pac} not found. Starting to install..."
        sudo yum install -y ${pac}
    done
    
    ### Install python packages by pip for the DB viewer
    for pac in ${pippackages[@]}; do
        echo "${pac} not found. Starting to install..."
        sudo pip3 install ${pac}
    done
    
    ### Confirmation
    /usr/bin/env python3 ${shell_dir}/check_python_modules.py
    if [ $? = 1 ]; then
        echo -e "[LDB] Failed, exit..."
        exit
    fi
    echo -e "[LDB] Done."
    echo -e "[LDB]"
    echo -e "[LDB]Finished installation!!"
    echo -e "[LDB]Install log can be found in: ${LOGFILE}"
fi

readme=${shell_dir}/setting/README.md

echo -e "----------------------------------------"
echo -e "# Local DB Quick Tutorial for DAQ Server" | tee ${readme}
echo -e "" | tee -a ${readme}
echo -e "## 1. Setup database config and function" | tee -a ${readme}
echo -e "\`\`\`" | tee -a ${readme}
echo -e "./setup_db.sh" | tee -a ${readme}
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
echo -e "./bin/scanConsole -c configs/connectivity/example_fei4b_setup.json -r configs/controller/emuCfg.json -s configs/scans/fei4b/std_digitalscan.json -W" | tee -a ${readme}
echo -e "\`\`\`" | tee -a ${readme}
echo -e "" | tee -a ${readme}
echo -e "## 4. Check results in the DB viewer in your web browser" | tee -a ${readme}
echo -e "- From the DB machine: http://localhost:5000/localdb/" | tee -a ${readme}
echo -e "- From other machines : http://${ip}/localdb/" | tee -a ${readme}
echo -e "" | tee -a ${readme}
echo -e "## 5.Check more detail" | tee -a ${readme}
echo -e "- https://github.com/jlab-hep/Yarr/wiki" | tee -a ${readme}
echo -e "----------------------------------------"
echo -e "This description is saved as ${readme}. Enjoy!!"
