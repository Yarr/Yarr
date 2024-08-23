#!/bin/bash

##
## Script to run through the steps to tune an ITkPixV1 ship.
##
## author: Liam Foster
## author: Daniel Joseph Antrim
## e-mail: liam@lbl.gov
## date: June 2024
##

function print_usage {
    echo "---------------------------------------------------"
    echo " Tune Rd53b"
    echo ""
    echo " Usage:"
    echo "  $ ${1}  -t <threshold> -r <controller> -c <connectivity>"
    echo ""
    echo " Options:"
    echo "  -t         Threshold the chip will be tuned to [default: 1000e]."
    echo "  -r          Path to JSON YARR controller [REQUIRED]."
    echo "  -c          Path to JSON YARR connectivity [REQUIRED]."
    echo "  -o          Output directory [default: ./data/]."
    echo "  -h|--help   Print this help message and exit."
    echo ""
    echo "---------------------------------------------------"
}


function main {

    threshold="1000"
    controller=""
    connectivity=""
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
            -t)
                threshold=${2}
                shift
                ;;
            -r)
                controller=${2}
                shift
                ;;
            -c)
                connectivity=${2}
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
    if [ "${controller}" == "" ]; then
        echo "ERROR No controller configuration file provided"
        return 1
    else
        if [ ! -f ${controller} ]; then
            echo "ERROR Could not find path to indicated controller configuration file (=${controller})"
            return 1
        fi
    fi

    if [ "${connectivity}" == "" ]; then
        echo "ERROR No connectivity configuration file provided"
        return 1
    else
        if [ ! -f ${connectivity} ]; then
            echo "ERROR Could not find path to indicated connectivity configuration file (=${connectivity})"
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

    ./bin/eyeDiagram -r ${controller} -c ${connectivity}

    base_cmd="${scan_console} -r ${controller} -c ${connectivity} -o ${output_dir}"
    

    # threshold tuning
    ${base_cmd} -s ${scan_dir}/std_tune_globalthreshold.json -t $(( ${threshold} + 200 ))
    ${base_cmd} -s ${scan_dir}/std_tune_pixelthreshold.json -t ${threshold}

    # after-tuning threshold distribution
    ${base_cmd} -s ${scan_dir}/std_thresholdscan.json
    ./bin/plotFromDir -i data/last_scan/ -p png -P

    # analog scan currently a bit buggy, so -t 5000
    ${base_cmd} -s ${scan_dir}/std_analogscan.json -t 5000
    ./bin/plotFromDir -i data/last_scan/ -p png -P
}

#______________________________________
main $*
