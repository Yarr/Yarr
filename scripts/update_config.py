'''
Script to update exisiting chip configs from 640 Mbps to 1.28 Gbps 
Mainly targeted at moduleQC 

Author: Maria Mironova (maria.mironova@cern.ch)
        Laura Nosler (laura.clare.nosler@cern.ch)

usage: scripts/update_config.py [-h] [-c CONFIG_FILE] [-r CONTROLLER_FILE] [-t CHIP_TYPE]

optional arguments:
  -h, --help            show this help message and exit
  -c CONNECTIVITY_FILE, --connectivity CONNECTIVITY_FILE
                        Connectivity file
  -r CONTROLLER_FILE, --controller CONTROLLER_FILE
                        Controller file
'''

import json 
import sys
import os
import argparse

def get_setup_type(current_tx,occurrence):
    if occurrence.get(current_tx) == 1:
        return "SCC"
    elif occurrence.get(current_tx) == 4:
        return "quad"
    else:
        return "ERROR"

def update_config(connectivity_file,controller_file):
    # Get controller type
    hw = open(controller_file)
    data_hw = json.load(hw)
    hw_type = data_hw["ctrlCfg"]["type"]
    if (hw_type != "spec" and hw_type != "FelixClient"):
        print("ERROR: Invalid controller file - only spec or FelixClient controllers supported")
    else:
        print("Updating config to",hw_type,"specifications")

    # Opening JSON file
    f = open(connectivity_file)
    # returns JSON object as 
    # a dictionary
    data = json.load(f)
    chip_type=data["chipType"]
    dir_path=os.path.split(connectivity_file)[0]

    # identify the setup types (SCC vs quad)
    tx = []
    for j in range(0,len(data["chips"])):
        chip=data["chips"][j]
        tx.append(chip["tx"])
    occurrence = {item: tx.count(item) for item in tx} # 1 for SCC, 4 for quad

    for j in range(0,len(data["chips"])):
        current_type = get_setup_type(tx[j],occurrence)
        if (current_type == "ERROR"):
            print("Unsupported type - only SCC or Quad Module supported")
            continue
        
        chip=data["chips"][j]
        if ("path" in chip.keys()):
            if (chip["path"] == "relToExec"): 
                chipConfigPath = chip["config"]
            elif (chip["path"] == "relToCon"): 
                chipConfigPath = dir_path + "/" + chip["config"]
            elif (chip["path"] == "abs"):
                chipConfigPath = chip["config"]
            elif (chip["path"] == "relToYarrPath"): 
                yarr_path = os.getcwd()
                chipConfigPath = yarr_path + "/" + chip["config"]
        else: 
            # Otherwise assume chip configs live in "YARR/configs/"
            config_path=dir_path.split("configs/")[0]
            chipConfigPath = config_path+chip["config"]


        print("Updating chip config %s"%(chipConfigPath))
        f_chip=open(chipConfigPath)
        data_chip=json.load(f_chip)
        if (hw_type == "spec"):
            data_chip[chip_type]["GlobalConfig"]["CdrClkSel"]=0
            data_chip[chip_type]["GlobalConfig"]["CmlBias0"]=800
            data_chip[chip_type]["GlobalConfig"]["CmlBias1"]=400
            data_chip[chip_type]["GlobalConfig"]["MonitorV"]=32
            data_chip[chip_type]["GlobalConfig"]["MonitorEnable"]=1

        elif (hw_type == "FelixClient"):
            data_chip[chip_type]["GlobalConfig"]["CdrClkSel"]=0
            data_chip[chip_type]["GlobalConfig"]["CmlBias0"]=800
            data_chip[chip_type]["GlobalConfig"]["CmlBias1"]=0
            data_chip[chip_type]["GlobalConfig"]["ServiceBlockEn"]=0
            data_chip[chip_type]["Parameter"]["EnforceNameIdCheck"] = False
            if (current_type=="SCC"):
                data_chip[chip_type]["GlobalConfig"]["DataMergeOutMux0"]=0
                data_chip[chip_type]["GlobalConfig"]["DataMergeOutMux1"]=0
                data_chip[chip_type]["GlobalConfig"]["DataMergeOutMux2"]=0
                data_chip[chip_type]["GlobalConfig"]["DataMergeOutMux3"]=0
                data_chip[chip_type]["GlobalConfig"]["SerEnLane"]=15
                
        with open(chipConfigPath,'w') as outfile:
            outfile.write(json.dumps(data_chip, sort_keys=True, indent=4))
        outfile.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--connectivity', dest="connectivity_file", help="Connectivity file")
    parser.add_argument('-r', '--controller', dest="controller_file", help="HW controller file")
    args = vars(parser.parse_args())

    connectivity_file=args["connectivity_file"]
    controller_file=args["controller_file"]
    update_config(connectivity_file,controller_file)