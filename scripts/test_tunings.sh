#!/bin/bash

bin/scanConsole -r configs/sw_tests/sw_test_emuCfg_rd53a.json -c configs/connectivity/example_rd53a_setup.json -s configs/sw_tests/sw_test_std_digitalscan_rd53x.json -p || exit 1
bin/scanConsole -r configs/sw_tests/sw_test_emuCfg_rd53a.json -c configs/connectivity/example_rd53a_setup.json -s configs/sw_tests/sw_test_std_analogscan_rd53x.json -p || exit 1
bin/scanConsole -r configs/sw_tests/sw_test_emuCfg_rd53a.json -c configs/connectivity/example_rd53a_setup.json -s configs/sw_tests/sw_test_std_thresholdscan_rd53x.json -p || exit 1
echo "Done"
