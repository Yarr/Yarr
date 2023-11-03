import json 
import sys
import os

config_file=sys.argv[1]

# Opening JSON file
f = open(config_file)
  
# returns JSON object as 
# a dictionary
data = json.load(f)

for j in range(0,len(data["chips"])):
    print("Configuring Chip %s"%j)
    chip_config=data["chips"][j]['config']
    f_chip=open(chip_config)
    data_chip=json.load(f_chip)
    data_chip["RD53B"]["GlobalConfig"]["CdrClkSel"]=0
    data_chip["RD53B"]["GlobalConfig"]["CmlBias0"]=800
    data_chip["RD53B"]["GlobalConfig"]["CmlBias1"]=400
    data_chip["RD53B"]["GlobalConfig"]["MonitorV"]=32

    data_chip["RD53B"]["GlobalConfig"]["MonitorEnable"]=1

    with open(chip_config,'w') as outfile:
        outfile.write(json.dumps(data_chip, sort_keys=True, indent=4))
    outfile.close()


