'''
Script to restore chip configs to untuned state (removing PixelConfig and DiffTh1L/R/M register settings from configs). For use in moduleQC.

Author: Emily Thompson (emily.anne.thompson@cern.ch)

usage: scripts/update_chip_dbconfig.py [-h] [-c CONFIG_FILE]

optional arguments:
  -h, --help            show this help message and exit
  -c CONNECTIVITY_FILE, --connectivity CONNECTIVITY_FILE
                        Connectivity file
'''

import json 
import sys
import os
import argparse

def clear_chip_config(connectivity_file):

    # Opening JSON file
    f = open(connectivity_file)
    # returns JSON object as 
    # a dictionary
    data = json.load(f)
    chip_type=data["chipType"]
    if (chip_type != "RD53B" and chip_type != "ITKPIXV2"):
        sys.exit("ERROR: Invalid chip type - only RD53B or ITKPIXV2 supported")
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
            # Otherwise assume chip configs live in "YARR/configs/"
            config_path=dir_path.split("configs/")[0]
            chipConfigPath = config_path+chip["config"]


        print("Updating chip config %s"%(chipConfigPath))
        f_chip=open(chipConfigPath)
        data_chip=json.load(f_chip)

        if data_chip[chip_type].get("PixelConfig"):
            del data_chip[chip_type]["PixelConfig"]

        for th in ["DiffTh1L", "DiffTh1M", "DiffTh1R"]:
            if th in data_chip[chip_type]["GlobalConfig"].keys():
                del data_chip[chip_type]["GlobalConfig"][th]

                
        with open(chipConfigPath,'w') as outfile:
            outfile.write(json.dumps(data_chip, sort_keys=True, indent=4))
        outfile.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--connectivity', dest="connectivity_file", help="Connectivity file")
    args = vars(parser.parse_args())

    connectivity_file=args["connectivity_file"]
    clear_chip_config(connectivity_file)
