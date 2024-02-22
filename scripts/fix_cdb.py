#!/usr/bin/env python3
import json

# Opening JSON file
f = open('checks/compilation_cmds.json')
 
# returns JSON object as
# a dictionary
units=list()
data = json.load(f)
f.close()
for unit in data:
    if  "directory" in unit:
        if unit["directory"].find("/external") == -1:
            units.append(unit)
    else:
        units.append(unit)
    
print(json.dumps(units, indent=4))
