#!/bin/sh

mod_id=$1
conn_path=$2
now=`date +"%y%m%d%H%M"`

if [ $# = 1 ]; then
    echo "Write Database ... [ serial number: $mod_id, userIdentity: $USER, institution: $HOSTNAME ]"
    echo -n "Continue ... [y/n] "
    read answer
    if [ $answer = "n" ]; then
        exit
    fi
elif [ $# = 2 ]; then
    echo "Write Database ... [ serial number: $mod_id, environmental configuration: $conn_path ]"
    echo -n "Continue ... [y/n] "
    read answer
    if [ $answer = "n" ]; then
        exit
    fi
else 
    echo "-> Error while parsing command line parameters"
    echo "usage: ./create_fei4b_4chip_config.sh <serial number> [<environmental configuration>]"
    exit
fi

# Change fixed tx channel and rx start channel
tx_fix=4
rx_start=4

for i in 1 2 3 4
do
    if [ -e ${mod_id}_fei4b_chipId$i.json ]; then
      echo "${mod_id}_fei4b_chipId$i.json is already exist, then move it in backup directory"
      cp ${mod_id}_fei4b_chipId$i.json backup/${mod_id}_fei4b_chipId$i.json.${now}
    fi
    echo "Create ${mod_id}_fei4b_chipId$i.json."
    cp default_fei4b.json ${mod_id}_fei4b_chipId$i.json
    sed -i "/chipId/s/0/$i/g" ${mod_id}_fei4b_chipId$i.json
    sed -i "/            \"rxChannel\": 99,/d" ${mod_id}_fei4b_chipId$i.json
    sed -i "/            \"txChannel\": 99,/d" ${mod_id}_fei4b_chipId$i.json
    j=`expr $i - 1 + $rx_start`
    sed -i "/rxChannel/s/0/$j/g" ${mod_id}_fei4b_chipId$i.json
    sed -i "/txChannel/s/0/$tx_fix/g" ${mod_id}_fei4b_chipId$i.json
    sed -i "/\"name\"/s/Example/${mod_id}_chipId$i/g" ${mod_id}_fei4b_chipId$i.json
done

echo "Create ${mod_id}_fei4module.json."
sed "s/__MODULE_ID__/${mod_id}/g" connectivity/fei4module_default.json > connectivity/${mod_id}_fei4module.json

cd ../

# Register module and 4 chips component to DB
if [ $# = 1 ]; then
    echo "./bin/dbRegisterComponent -c configs/connectivity/${mod_id}_fei4module.json"
    ./bin/dbRegisterComponent -c configs/connectivity/${mod_id}_fei4module.json
elif [ $# = 2 ]; then
    echo "./bin/dbRegisterComponent -c configs/connectivity/${mod_id}_fei4module.json -I configs/${conn_path}"
    ./bin/dbRegisterComponent -c configs/connectivity/${mod_id}_fei4module.json -I configs/${conn_path}
fi
