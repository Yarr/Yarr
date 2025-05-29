#!/bin/bash

function runScan ()
{
    bin/scanConsole -r ${CONTROLLER} -c ${CONNECTIVITY} -s ${SCAN_CFG}
    if [ $? -eq 0 ]; then
        echo ${scan} OK
    else
        echo ${scan} "FAILED (return code $?)"
        exit 1
    fi
}

CONNECTIVITY=configs/connectivity/example_star_setup_ppb.json

CONTROLLER=configs/controller/starEmu/emuCfg_star_gen_trim.json
SCAN_CFG=configs/scans/star/std_tune_trim_at_pedestal.json

runScan

CONTROLLER=configs/controller/starEmu/emuCfg_star_gen_bvt.json
SCAN_CFG=configs/scans/star/std_vt50scan.json

runScan

CONTROLLER=configs/controller/emuCfg_star_ppb.json
SCAN_CFG=configs/scans/star/std_latencyScan.json

runScan

