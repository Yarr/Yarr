#!/usr/bin/env python3
#################################
# Author: Eunchong Kim
# Email: eunchong.kim at cern.ch
# Date: April 2019
# Project: Local Database for YARR
# Description: Check python modules
#################################

from __future__ import print_function # Use print() in python2 and 3
import os, sys
import pkg_resources

module_names = []
requirement_file = os.path.dirname(os.path.abspath(__file__))+"/requirements-pip.txt"
if os.path.isfile(requirement_file):
    for line in open(requirement_file):
        if line[0] != '#':
            module_names.append(line.strip())
else:
    print("ERROR! Cannot open 'requirements-pip.txt'")
    exit(1)

print("[LDB] Welcome to Local Database Tools!")

# Check python modules
def checkPythonModule():
    print("[LDB] Check python modules: ")
    founds = []
    
    packages = []
    for dist in pkg_resources.working_set:
        packages.append(dist.project_name)

    for module_name in module_names:
        if module_name in packages:
            founds.append(True)
        else:
            founds.append(False)

    for idx, module_name in enumerate(module_names):
        if idx == 0:
            print("\t" + module_names[idx] + "...", end = "")
        else:
            print("\t" + module_names[idx] + "...", end = "")
        if founds[idx] == False:
            print("not found!")
        else:
            print("OK!")
    
    if False in founds:
        print("[LDB] Error checking python modules! Exit!")
        exit(1)


if __name__ == '__main__':
    checkPythonModule()
