#!/bin/bash

bin/scanConsole -r configs/controller/test_emuCfg_rd53a.json -c configs/connectivity/example_rd53a_setup.json -s configs/scans/rd53a/test_std_digitalscan.json -p
bin/scanConsole -r configs/controller/test_emuCfg_rd53a.json -c configs/connectivity/example_rd53a_setup.json -s configs/scans/rd53a/test_std_analogscan.json -p
bin/scanConsole -r configs/controller/test_emuCfg_rd53a.json -c configs/connectivity/example_rd53a_setup.json -s configs/scans/rd53a/test_std_thresholdscan.json -p
bin/test_main
