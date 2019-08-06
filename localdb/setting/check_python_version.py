#!/usr/bin/env python3
#################################
# Author: Eunchong Kim
# Email: eunchong.kim at cern.ch
# Date: April 2019
# Project: Local Database for YARR
# Description: Check python modules
#################################

from __future__ import print_function # Use print() in python2 and 3
import os, sys, six
import pkg_resources

def checkPythonVersion():
    print("[LDB] Check Python version ... " + str(sys.version_info[0]) + "." + str(sys.version_info[1]) + " ... ", end = "")
    if six.PY34:
        print("OK!")
    else:
        print("Must use Python 3!")
        exit(1)

if __name__ == '__main__':
    checkPythonVersion()
