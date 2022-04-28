#!/usr/bin/env python3
import sys
import pyYARR
import json

# pass the scanConsole command line arguments to C++
# and get a json configuration CLOB back
if __name__ == '__main__':
    res = pyYARR.parseConfig(sys.argv)
    res = json.loads(res)
    print(res)
