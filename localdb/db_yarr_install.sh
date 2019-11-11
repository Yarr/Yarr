#!/bin/bash
#################################
# Contacts: Arisa Kubota
# Email: arisa.kubota at cern.ch
# Date: July 2019
# Project: Local Database for Yarr
# Description: Login Database 
# Usage: ./db_yarr_install.sh
################################

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

## Set log file
LOGDIR="${shell_dir}/setting/instlog"
if [ ! -d ${LOGDIR} ]; then
    mkdir ${LOGDIR}
fi
LOGFILE="${LOGDIR}/`date "+%Y%m%d_%H%M%S"`"
exec 2> >(awk '{print strftime("[%Y-%m-%d %H:%M:%S] "),$0 } { fflush() } ' | tee ${LOGFILE}) 1>&2
trap 'echo -e ""; echo -e "[LDB] Installation stopped by SIGINT!!"; echo -e "[LDB] You may be in unknown state."; echo -e "[LDB] Check ${LOGFILE} for debugging in case of a problem of re-executing this script."; exit 1' 2
    
#############################################
# Confirmation before starting installation #
#############################################

echo -e "[LDB] Looking for missing pachages for Local DB and Tools ..."

# Check python version
echo -e "[LDB]     Checking Python version ..."
pytver=false
if which python3 > /dev/null 2>&1; then
    python3 <<EOF
import sys
if not sys.version_info[0]==3: sys.exit(1)
if not sys.version_info[1]>=4: sys.exit(1)
sys.exit(0)
EOF
    if [ $? = 0 ]; then
        pytver=true
    fi
fi

# Check python package
echo -e "[LDB]     Checking Python packages ..."
pippackages=$(cat ${shell_dir}/setting/requirements-pip.txt)
if "${pytver}"; then
    for pac in ${pippackages[@]}; do
        if ! python3 -m pip list 2>&1 | grep ${pac} 2>&1 > /dev/null; then
            piparray+=(${pac})
        fi
    done
    pippackages=${piparray[@]}
fi

# Check yum package
echo -e "[LDB]     Checking yum packages ..."
yumpackages=$(cat ${shell_dir}/setting/requirements-yum.txt)
for pac in ${yumpackages[@]}; do
    if "${pytver}" && [ `echo ${pac} | grep 'python'` ]; then
        continue
    fi
    if ! yum list installed 2>&1 | grep ${pac} > /dev/null; then
        yumarray+=(${pac})
    fi
done
yumpackages=${yumarray[@]}

echo "[LDB] Done."

### Confirmation
if [ ${#yumpackages} != 0 ] || [ ${#pippackages} != 0 ]; then
    printf '\033[33m%s\033[m\n' "------------------------------------"
    printf '\033[33m%s\033[m\n' "          Missing packages          "
    printf '\033[33m%s\033[m\n' "------------------------------------"
    if [ ${#yumpackages} != 0 ]; then
        for pac in ${yumpackages[@]}; do
            printf '\033[33m%s\033[m\n' " yum    : ${pac}"
        done
    fi
    if [ ${#pippackages} != 0 ]; then
        for pac in ${pippackages[@]}; do
            printf '\033[33m%s\033[m\n' " python : ${pac}"
        done
    fi
    printf '\033[33m%s\033[m\n' "------------------------------------"
    echo -e ""
    echo -e "[LDB] Install these package? [y/n]"
    unset answer
    while [ -z ${answer} ]; do
        read -p "" answer
    done
    echo -e ""
    if [ ${answer} != "y" ]; then
        echo -e "[LDB] Exit..."
        echo -e ""
        echo -e "[LDB] If you want to setup them manually, the page 'https://localdb-docs.readthedocs.io/en/master/' should be helpful!"
        echo -e ""
        exit
    fi

    echo -e "[LDB] You need to be root to perform this command."
    sudo echo -e "[LDB] OK!"
    if [ $? = 1 ]; then
        printf '\033[31m%s\033[m\n' '[LDB ERROR] Failed, exit ...'
        exit
    fi

    ### Install necessary packages if not yet installed
    echo -e "[LDB] Start installing necessary packages..."
    
    ### Install yum packages
    for pac in ${yumpackages[@]}; do
        echo -e "[LDB]     Installing ${pac} ..."
        sudo yum install -y ${pac}
    done
    
    ### Install python packages by pip for the DB viewer
    for pac in ${pippackages[@]}; do
        echo -e "[LDB]     Installing ${pac} ..."
        sudo pip3 install ${pac}
    done
    
    echo -e "[LDB] Done."
    echo -e "[LDB]"
    echo -e "[LDB] Finished installation!!"
    echo -e "[LDB] Install log can be found in: ${LOGFILE}"
    echo -e "[LDB]"
else
    echo -e "[LDB]"
    echo -e "[LDB] Requirement already satisfied."
    echo -e "[LDB]"
fi

# Final Confirmation
echo -e "[LDB] Final confirming ..."
pytver=false
if which python3 > /dev/null 2>&1; then
    python3 <<EOF
import sys
if not sys.version_info[0]==3: sys.exit(1)
if not sys.version_info[1]>=4: sys.exit(1)
sys.exit(0)
EOF
    if [ $? = 0 ]; then
        pytver=true
    fi
fi

pippackages=$(cat ${shell_dir}/setting/requirements-pip.txt)
if "${pytver}"; then
    unset piparray
    for pac in ${pippackages[@]}; do
        if ! python3 -m pip list 2>&1 | grep ${pac} 2>&1 > /dev/null; then
            piparray+=(${pac})
        fi
    done
    pippackages=${piparray[@]}
fi

yumpackages=$(cat ${shell_dir}/setting/requirements-yum.txt)
for pac in ${yumpackages[@]}; do
    unset yumarray
    if "${pytver}" && [ `echo ${pac} | grep 'python'` ]; then
        continue
    fi
    if ! yum list installed 2>&1 | grep ${pac} > /dev/null; then
        yumarray+=(${pac})
    fi
done
yumpackages=${yumarray[@]}

if ! "${pytver}" || [ ${#yumpackages} != 0 ] || [ ${#pippackages} != 0 ]; then
    if ! "${pytver}"; then
        printf '\033[31m%s\033[m\n' '[LDB ERROR] Python version 3.4 or greater is required.'
    fi
    if [ ${#yumpackages} != 0 ]; then
        printf '\033[31m%s\033[m\n' '[LDB ERROR] yum packages not be installed: '
        for pac in ${yumpackages[@]}; do
            printf '\033[31m%s\033[m\n' "    - ${pac}"
        done
    fi
    if [ ${#pippackages} != 0 ]; then
        printf '\033[31m%s\033[m\n' '[LDB ERROR] python packages not be installed: '
        for pac in ${pippackages[@]}; do
            printf '\033[31m%s\033[m\n' "    - ${pac}"
        done
    fi
else
    echo -e "[LDB] Success!!"
    echo -e ""
    echo -e "----------------------------------------"
    echo -e "# Quick Tutorial for DAQ with Local DB"
    echo -e ""
    echo -e "## 1. Setup database config and function"
    echo -e "$ ./setup_db.sh" 
    echo -e ""
    echo -e "## 2. Setup scanConsole"
    echo -e "$ cd YARR"
    echo -e "$ mkdir build"
    echo -e "$ cmake3 ../" 
    echo -e "$ make -j4" 
    echo -e "$ make install" 
    echo -e ""
    echo -e "## 3. Scan with Local DB"
    echo -e "$ ./bin/scanConsole -c configs/connectivity/example_fei4b_setup.json -r configs/controller/emuCfg.json -s configs/scans/fei4b/std_digitalscan.json -W" 
    echo -e ""
    echo -e "## 4. Check if the upload is success in log file: ${HOME}/.yarr/localdb/log/###.log"
    echo -e ""
    echo -e "## 5.Check more detail"
    echo -e "- https://localdb-docs.readthedocs.io/en/master/"
    echo -e "----------------------------------------"
    echo -e "Enjoy!!"
fi
