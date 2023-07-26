#!/bin/bash

##
## Script to run through the steps to tune an ITkPixV1 ship.
##
## author: Daniel Joseph Antrim
## e-mail: daniel.joseph.antrim AT cern DOT ch
## date: March 2021
##

function print_usage {
    echo "---------------------------------------------------"
    echo " Tune Rd53b"
    echo ""
    echo " Usage:"
    echo "  $ ${1}  --t0 <threshold first> --t1 <threshold last> -r <controller> -c <connectivity>"
    echo ""
    echo " Options:"
    echo "  --t0        Initial threshold that the chip will be tuned to in the first pass [default: 1500e]."
    echo "  --t1        Final threshold that the chip will be tuned to [default: 1000e]."
    echo "  -r          Path to JSON YARR controller configuration file [REQUIRED]."
    echo "  -c          Path to JSON YARR connectivity configuration file [REQUIRED]."
    echo "  -o          Output directory [default: ./data/]."
    echo "  -h|--help   Print this help message and exit."
    echo ""
    echo "---------------------------------------------------"
}


function main {

    first_threshold="1500"
    second_threshold="1000"
    controller_config=""
    connectivity_config=""
    output_dir="./data/"

    while test $# -gt 0
    do
        case $1 in
            -h)
                print_usage $0
                return 0
                ;;
            --help)
                print_usage $0
                return 0
                ;;
            --t0)
                first_threshold=${2}
                shift
                ;;
            --t1)
                second_threshold=${2}
                shift
                ;;
            -r)
                controller_config=${2}
                shift
                ;;
            -c)
                connectivity_config=${2}
                shift
                ;;
            -o)
                output_dir=${2}
                shift
                ;;
            *)
                echo "ERROR Invalid argument provided: ${1}"
                return 1
                ;;
        esac
        shift
    done

    ##
    ## check inputs
    ##
    if [ "${controller_config}" == "" ]; then
        echo "ERROR No controller configuration file provided"
        return 1
    else
        if [ ! -f ${controller_config} ]; then
            echo "ERROR Could not find path to indicated controller configuration file (=${controller_config})"
            return 1
        fi
    fi

    if [ "${connectivity_config}" == "" ]; then
        echo "ERROR No connectivity configuration file provided"
        return 1
    else
        if [ ! -f ${connectivity_config} ]; then
            echo "ERROR Could not find path to indicated connectivity configuration file (=${connectivity_config})"
            return 1
        fi
    fi

    ##
    ## run tuning procedure
    ##

    scan_console="./bin/scanConsole"
    if [ ! -f ${scan_console} ]; then
        echo "ERROR Could not find scanConsole executable (=${scan_console}) in current working directory"
        return 1
    fi

    scan_dir="./configs/scans/rd53b"
    if [ ! -d ${scan_dir} ]; then
        echo "ERROR Scan configuration directory (=${scan_dir}) not found in current working directory"
        return 1
    fi

    base_cmd="${scan_console} -r ${controller_config} -c ${connectivity_config} -o ${output_dir}"

    ${base_cmd} -s ${scan_dir}/std_digitalscan.json -m 1
    ${base_cmd} -s ${scan_dir}/std_analogscan.json

    # before-tuning threshold distribution
    ${base_cmd} -s ${scan_dir}/std_thresholdscan.json -p

    # initial pass tuning
    ${base_cmd} -s ${scan_dir}/std_tune_globalthreshold.json -t ${first_threshold} -p
    ${base_cmd} -s ${scan_dir}/std_tune_pixelthreshold.json -t ${first_threshold} -p

    # second pass tuning
    ${base_cmd} -s ${scan_dir}/std_retune_globalthreshold.json -t ${second_threshold} -p
    ${base_cmd} -s ${scan_dir}/std_retune_pixelthreshold.json -t ${second_threshold} -p

    # after-tuning threshold distribution
    ${base_cmd} -s ${scan_dir}/std_thresholdscan.json -p
}

#______________________________________
main $*
