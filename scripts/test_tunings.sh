#!/bin/bash
for scan in std_digitalscan std_analogscan std_thresholdscan
do
   bin/scanConsole -r configs/sw_tests/sw_test_emuCfg_rd53a.json -c configs/connectivity/example_rd53a_setup.json -s configs/sw_tests/sw_test_${scan}_rd53x.json -p
   if [ $? -eq 0 ]; then
   echo ${scan} OK
else
   echo ${scan} "FAILED (return code $?)"
   exit 1
fi
done
