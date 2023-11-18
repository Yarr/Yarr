'''
Script to update exisiting chip configs from 640 Mbps to 1.28 Gbps 
Mainly targeted at moduleQC 

Author: Maria Mironova (maria.mironova@cern.ch)

usage: scripts/update_config.py [-h] [-c CONFIG_FILE] [-t CHIP_TYPE]

optional arguments:
  -h, --help            show this help message and exit
  -c CONNECTIVITY_FILE, --connectivity CONNECTIVITY_FILE
                        Connectivity file
'''

import json 
import sys
import os
import argparse



def update_config(connectivity_file):
    # Opening JSON file
    f = open(connectivity_file)
    
    # returns JSON object as 
    # a dictionary
    data = json.load(f)
    chip_type=data["chipType"]
    dir_path=os.path.split(connectivity_file)[0]
    for j in range(0,len(data["chips"])):
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
            chipConfigPath = chip["config"]


        print("Updating chip config %s"%(chipConfigPath))
        f_chip=open(chipConfigPath)
        data_chip=json.load(f_chip)
        data_chip[chip_type]["GlobalConfig"]["CdrClkSel"]=0
        data_chip[chip_type]["GlobalConfig"]["CmlBias0"]=800
        data_chip[chip_type]["GlobalConfig"]["CmlBias1"]=400
        data_chip[chip_type]["GlobalConfig"]["MonitorV"]=32
        data_chip[chip_type]["GlobalConfig"]["MonitorEnable"]=1

        with open(chipConfigPath,'w') as outfile:
            outfile.write(json.dumps(data_chip, sort_keys=True, indent=4))
        outfile.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--connectivity', dest="connectivity_file", help="Connectivity file")
    args = vars(parser.parse_args())

    connectivity_file=args["connectivity_file"]
    update_config(connectivity_file)