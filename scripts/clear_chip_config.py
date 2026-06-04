'''
Script to restore chip configs to untuned state (removing PixelConfig and DiffTh1L/R/M register settings from configs). For use in moduleQC.

Author: Emily Thompson (emily.anne.thompson@cern.ch), Lingxin Meng (lingxin.meng@cern.ch)

usage: scripts/clear_chip_config.py [-h] [-c CONNECTIVITY_FILE]

optional arguments:
  -h, --help            show this help message and exit
  -c CONNECTIVITY_FILE, --connectivity CONNECTIVITY_FILE
                        Connectivity file
'''

import json
import logging
import sys
import os
import argparse

log = logging.getLogger(os.path.basename(__file__))
logging.basicConfig(level=logging.INFO)

## copy function from module-qc-database-tools in order not to have dependency?
def get_layer_from_serial_number(serial_number):
    """
    Get the layer from the serial number.
    """
    if len(serial_number) != 14 or not serial_number.startswith("20U"):
        log.exception("Error: Please enter a valid ATLAS SN.")
        raise ValueError()
    YY = serial_number[5:7]
    if "B1" in YY or "FC" in YY:
        return (
            "L2"  ## Doesn't look like there is anything dependent on SCC vs module flex
        )

    if "PIMS" in serial_number or "PIR6" in serial_number:
        return "L0"

    if "PIM0" in serial_number or "PIR7" in serial_number:
        return "R0"

    if "PIM5" in serial_number or "PIR8" in serial_number:
        return "R0.5"

    if "PIM1" in serial_number or "PIRB" in serial_number:
        return "L1"

    if "PG" in serial_number:
        return "L2"

    log.exception("Invalid module SN: %s", serial_number)
    raise ValueError()

def clear_chip_config(connectivity_file):
    # check if any SN is in the connectivity file name
    sn = connectivity_file.split("/")[-1]
    isModule = False
    layer = ""
    if sn.startswith("20U"):
        try:
            layer = sn.split("_")[1]
        except IndexError:
            log.warning(f"No layer info found in the connecivity file name {connectivity_file}")

        sn = sn[:14]
        isModule = True
        

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


        log.info("Updating chip config %s"%(chipConfigPath))
        f_chip=open(chipConfigPath)
        data_chip=json.load(f_chip)

        if data_chip[chip_type].get("PixelConfig"):
            log.info("Deleting PixelConfig from chip config")
            del data_chip[chip_type]["PixelConfig"]

        for th in ["DiffTh1L", "DiffTh1M", "DiffTh1R", "DiffVff"]:
            if th in data_chip[chip_type]["GlobalConfig"].keys():
                log.info(f"Deleting {th} from chip config")
                del data_chip[chip_type]["GlobalConfig"][th]

        if isModule:
            if layer in ["R0", "R0.5", "L0", "L1", "L2", "LP"]:
                power_config = layer
            else:
                power_config = get_layer_from_serial_number(sn)
            log.info(f"Reset to default DiffVff for layer {power_config}.")
            data_chip[chip_type]["GlobalConfig"]["DiffVff"] = {
            "R0": 150,
            "R0.5": 150,
            "L0": 150,
            "L1": 150,
            "L2": 60,
            "LP": 0,
            }[power_config]

        with open(chipConfigPath,'w') as outfile:
            outfile.write(json.dumps(data_chip, sort_keys=True, indent=4))
        outfile.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--connectivity', dest="connectivity_file", help="Connectivity file")
    args = vars(parser.parse_args())

    connectivity_file=args["connectivity_file"]
    clear_chip_config(connectivity_file)
