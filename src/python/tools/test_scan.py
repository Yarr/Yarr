#!/usr/bin/env python3
import sys
import pyYARR
import json

def run_scan():
    con = pyYARR.ScanConsole()
    res = pyYARR.parseConfig(sys.argv)
    res = json.loads(res)
    if res['status'] == "false":
        return -1
    res = con.loadConfig(json.dumps(res))
    if res != 0:
        return res
    res = con.initHardware()
    if res != 0:
        return res
    res = con.configure()
    if res != 0:
        return res
    res = con.setupScan()
    if res != 0:
        return res
    con.run()
    res = con.getResults()
    print(res)
    print(json.loads(res))
    return 0

if __name__ == '__main__':
    run_scan()
