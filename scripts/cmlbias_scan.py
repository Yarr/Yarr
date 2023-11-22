"""
Script to run CmlBias scans 

Author: Maria Mironova (maria.mironova@cern.ch)

usage: cmlbias_scan.py [-h] [-c connectivity_file] [-r CONTROLLER_FILE]
                       [-o OUTPUT_DIRECTORY]

optional arguments:
  -h, --help            show this help message and exit
  -c connectivity_file, --connectivity connectivity_file         Chip Connectivity file
  -r CONTROLLER_FILE, --controller CONTROLLER_FILE   HW controller file
  -o OUTPUT_DIRECTORY, --output OUTPUT_DIRECTORY     Output directory
"""

import os 
import json
import argparse

def scan_cmlbias(connectivity_file, controller_file, directory):
    print(connectivity_file)
    with open(connectivity_file) as f:
        data = json.load(f)
    chip_type=data["chipType"]
    dir_path=os.path.split(connectivity_file)[0]

    if not os.path.exists(directory):
        os.makedirs(directory)

    for cmlbias0 in [500, 600, 700, 800, 900, 1000]:
        for cmlbias1 in [0, 200, 400, 600, 800]:                
            if cmlbias1 >= cmlbias0:
                continue
            else:
                print("Editing configs...")
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

                    data_chip[chip_type]["GlobalConfig"]["CmlBias0"] = cmlbias0

                    if cmlbias1 == 0:
                        data_chip[chip_type]["GlobalConfig"]["SerEnTap"] = 0
                    else:
                        data_chip[chip_type]["GlobalConfig"]["SerEnTap"] = 1
                    data_chip[chip_type]["GlobalConfig"]["CmlBias1"] = cmlbias1

                    with open(chipConfigPath, 'w') as outfile:
                        outfile.write(json.dumps(data_chip, sort_keys=True, indent=4))

                print("Running eye diagram...")
                print("./bin/eyeDiagram -r %s -c %s -n" % (controller_file, connectivity_file))
                os.system("./bin/eyeDiagram -r %s -c %s -n" % (controller_file, connectivity_file))
                os.system("cp results.txt %s/results_%s_%s.txt" % (directory, cmlbias0, cmlbias1))

    # Reset to defaults
    for j in range(0,len(data["chips"])):
        chip_config = data["chips"][j]['config']
        with open(chip_config) as f_chip:
            data_chip = json.load(f_chip)
        data_chip[chip_type]["GlobalConfig"]["SerEnTap"] = 1
        data_chip[chip_type]["GlobalConfig"]["CmlBias0"] = cmlbias0
        data_chip[chip_type]["GlobalConfig"]["CmlBias1"] = cmlbias1
        with open(chip_config, 'w') as outfile:
            outfile.write(json.dumps(data_chip, sort_keys=True, indent=4))



if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--connectivity', dest="connectivity_file", help="Connectivity file")
    parser.add_argument('-r', '--controller', dest="controller_file", default="configs/controller/specCfg-rd53b-16x1.json", help="HW controller file")
    parser.add_argument('-o', '--output', dest="output_directory", default="cmlbias_results", help="Output directory")
    args = vars(parser.parse_args())

    connectivity_file = args["connectivity_file"]
    controller_file = args["controller_file"]
    directory = args["output_directory"]

    scan_cmlbias(connectivity_file, controller_file, directory)