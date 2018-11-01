#!/bin/sh

mod_id=$1
scan_type=$2

stage='encapsulation' # {wirebond, encapsulation}

for i in 1 2 3 4
do
    if [ ! -e configs/${mod_id}_fei4b_chipId$i.json ]; then
  echo "File not found : configs/${mod_id}_fei4b_chipId$i.json"
  exit
    fi
done

./bin/scanConsole -r configs/specCfg.json -c configs/${mod_id}_fei4b_chipId1.json -c configs/${mod_id}_fei4b_chipId2.json -c configs/${mod_id}_fei4b_chipId3.json -c configs/${mod_id}_fei4b_chipId4.json -p -W ${mod_id} -t 2668 9 14941 -s ${scan_type} -F ${stage}
