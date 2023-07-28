import os 
import json
import argparse

def scan_cmlbias(config_file, controller_file, directory):

    with open(config_file) as f:
        data = json.load(f)

    if not os.path.exists(directory):
        os.makedirs(directory)

    for cmlbias0 in [500, 600, 700, 800, 900, 1000]:
        for cmlbias1 in [0, 200, 400, 600, 800]:                
            if cmlbias1 >= cmlbias0:
                continue
            else:
                print("Editing configs...")
                for j in range(0,len(data["chips"])):
                    chip_config = data["chips"][j]['config']
                    with open(chip_config) as f_chip:
                        data_chip = json.load(f_chip)

                    data_chip["RD53B"]["GlobalConfig"]["CdrClkSel"] = 0
                    data_chip["RD53B"]["GlobalConfig"]["MonitorV"] = 32
                    data_chip["RD53B"]["GlobalConfig"]["CmlBias0"] = cmlbias0

                    if cmlbias1 == 0:
                        data_chip["RD53B"]["GlobalConfig"]["SerEnTap"] = 0
                    else:
                        data_chip["RD53B"]["GlobalConfig"]["SerEnTap"] = 1
                    data_chip["RD53B"]["GlobalConfig"]["CmlBias1"] = cmlbias1

                    with open(chip_config, 'w') as outfile:
                        outfile.write(json.dumps(data_chip, sort_keys=True, indent=4))

                print("Running eye diagram...")
                os.system("./bin/eyeDiagram -r %s -c %s -s" % (controller_file, config_file))
                os.system("cp results.txt %s/results_%s_%s.txt" % (directory, cmlbias0, cmlbias1))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Run CmlBias scan.')
    parser.add_argument('-c', '--connectivity', dest="config_file", help="Connectivity file")
    parser.add_argument('-r', '--controller', dest="controller_file", default="configs/controller/specCfg-rd53b-16x1.json", help="HW controller file")
    parser.add_argument('-o', '--output', dest="directory", default="cmlbias_results", help="Output directory")
    args = vars(parser.parse_args())

    config_file = args["config_file"]
    controller_file = args["controller_file"]
    directory = args["directory"]

    scan_cmlbias(config_file, controller_file, directory)