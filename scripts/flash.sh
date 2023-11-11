#!/bin/bash

HW=("tef1001_R1" "tef1001_R2" "xpressk7_160" "xpressk7_325")
SPEED=("1280Mbps" "640Mbps")
CHANNELS=("16x1" "4x4")

echo "For further information on how to answer the following questions please check: todo"

# FPGA card
for((i=0;i<${#HW[@]};i++))
do
    echo "$i) ${HW[$i]}"
done
read -p 'Choose FPGA card: ' hw_index
if (($hw_index >= ${#HW[@]})); then
    echo "Answer out of range ... aborting!"
    exit 1
fi
echo "${HW[$hw_index]} chosen!"
echo -e "\n"

if [ $# -eq 0 ]
then
    echo "No arguments supplied, starting questionaire ..."
    echo -e "\n"

    # SPEED card
    for((i=0;i<${#SPEED[@]};i++))
    do
        echo "$i) ${SPEED[$i]}"
    done
    read -p 'Choose link speed: ' speed_index
    if (($speed_index >= ${#SPEED[@]})); then
        echo "Answer out of range ... aborting!"
        exit 1
    fi
    echo "${SPEED[$speed_index]} chosen!"
    echo -e "\n"

    # Channel config
    for((i=0;i<${#CHANNELS[@]};i++))
    do
        echo "$i) ${CHANNELS[$i]}"
    done
    read -p 'Choose channel config: ' channel_index
    if (($channel_index >= ${#CHANNELS[@]})); then
        echo "Answer out of range ... aborting!"
        exit 1
    fi
    echo "${CHANNELS[$channel_index]} chosen!"
    echo -e "\n"

    # Construct string
    DIR=rd53_ohio_${CHANNELS[$channel_index]}_${SPEED[$speed_index]}
    FILE=${DIR}_${HW[$hw_index]}.bit

    echo "Directory: $DIR"
    echo "File: $FILE"

    URL="http://cern.ch/yarr/firmware/latest/"

    # Download
    mkdir -p Downloads
    wget --backups -P ./Downloads ${URL}${DIR}/${FILE}

    BITFILE=Downloads/${FILE}
else
    BITFILE=$1
fi

read -p "Proceed to flash $BITFILE? [y/n]?" yn
if [[ ! $yn =~ ^[Yy]$ ]]
then
    echo "Not a clear answer, aborting!"
    exit 1
fi

# Find vivado
if type vivado >/dev/null 2>&1
then
    echo "Found vivado ..."
    VIV_EXEC=vivado
elif type vivado_lab >/dev/null 2>&1
then
    echo "Found vivado_lab ..."
    VIV_EXEC=vivado_lab
else
    echo "Can't find Vivado, did you source the script? ABORTING!"
    exit 1
fi

echo "Flashing: $BITFILE"
echo "Executeable: $VIV_EXEC"

if [ $hw_index -eq 0 ] || [ $hw_index -eq 1 ]; then
    DEVICE_INDEX="0"
    MEM_OPTIONS="-size 32 -interface SPIx4"
    MEM_TYPE="mt25qu256-spi-x1_x2_x4"
elif [ $hw_index -eq 2 ] || [ $hw_index -eq 3 ]; then
    DEVICE_INDEX="1"
    MEM_OPTIONS="-size 128 -interface BPIx16"
    MEM_TYPE="28f00ap30b-bpi-x16"
else
    echo "FPGA board not chosen ... aborting!"
    exit 1
fi

echo "Create tcl script ..."

echo "
open_hw
connect_hw_server
open_hw_target
current_hw_device [lindex [get_hw_devices] ${DEVICE_INDEX}]
refresh_hw_device -update_hw_probes false [lindex [get_hw_devices] ${DEVICE_INDEX}]
write_cfgmem  -format mcs ${MEM_OPTIONS} -loadbit \"up 0x00000000 ${BITFILE} \" -checksum -force -file \"mem.mcs\"
create_hw_cfgmem -hw_device [lindex [get_hw_devices] ${DEVICE_INDEX}] -mem_dev  [lindex [get_cfgmem_parts {${MEM_TYPE}}] 0]
set_property PROGRAM.BLANK_CHECK  0 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] ${DEVICE_INDEX} ]]
set_property PROGRAM.ERASE  1 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] ${DEVICE_INDEX} ]]
set_property PROGRAM.CFG_PROGRAM  1 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] ${DEVICE_INDEX} ]]
set_property PROGRAM.VERIFY  1 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] ${DEVICE_INDEX} ]]
set_property PROGRAM.CHECKSUM  0 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] ${DEVICE_INDEX} ]]
refresh_hw_device [lindex [get_hw_devices] ${DEVICE_INDEX}]
set_property PROGRAM.ADDRESS_RANGE  {use_file} [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] ${DEVICE_INDEX} ]]
set_property PROGRAM.FILES [list \"mem.mcs\" ] [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] ${DEVICE_INDEX}]]
set_property PROGRAM.PRM_FILE {mem.prm} [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] ${DEVICE_INDEX}]]
set_property PROGRAM.BPI_RS_PINS {none} [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] ${DEVICE_INDEX} ]]
set_property PROGRAM.UNUSED_PIN_TERMINATION {pull-none} [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] ${DEVICE_INDEX} ]]
set_property PROGRAM.BLANK_CHECK  0 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] ${DEVICE_INDEX} ]]
set_property PROGRAM.ERASE  1 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] ${DEVICE_INDEX} ]]
set_property PROGRAM.CFG_PROGRAM  1 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] ${DEVICE_INDEX} ]]
set_property PROGRAM.VERIFY  1 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] ${DEVICE_INDEX} ]]
set_property PROGRAM.CHECKSUM  1 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] ${DEVICE_INDEX} ]]
startgroup
if {![string equal [get_property PROGRAM.HW_CFGMEM_TYPE  [lindex [get_hw_devices] ${DEVICE_INDEX}]] [get_property MEM_TYPE [get_property CFGMEM_PART [get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] ${DEVICE_INDEX} ]]]]] }  { create_hw_bitstream -hw_device [lindex [get_hw_devices] ${DEVICE_INDEX}] [get_property PROGRAM.HW_CFGMEM_BITFILE [ lindex [get_hw_devices] ${DEVICE_INDEX}]]; program_hw_devices [lindex [get_hw_devices] ${DEVICE_INDEX}]; };
program_hw_cfgmem -hw_cfgmem [get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] ${DEVICE_INDEX} ]]
endgroup
current_hw_device [lindex [get_hw_devices] ${DEVICE_INDEX}]
refresh_hw_device -update_hw_probes false [lindex [get_hw_devices] ${DEVICE_INDEX}]
set_property PROGRAM.FILE {${BITFILE}} [lindex [get_hw_devices] ${DEVICE_INDEX}]
program_hw_devices [lindex [get_hw_devices] ${DEVICE_INDEX}]" > flash.tcl

echo "Start programming ..."

$VIV_EXEC -mode batch -source flash.tcl
