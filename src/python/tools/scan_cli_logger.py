#!/usr/bin/env python3
import sys
import pyYARR
import json

logConfig =  { 
              "pattern": "[%T:%e]%^[%=8l][%=15n]:%$ %v",
              "log_config": [ {"name": "all", "level" : "trace"} ]
}
 

pyYARR.setupLogger(json.dumps(logConfig))

def run_scan():
    con = pyYARR.ScanConsole()
    res = con.init(sys.argv)
    if res <= 0:
        return res
    res = con.loadConfig()
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
    con.cleanup()
    con.plot()
    return 0

if __name__ == '__main__':
    run_scan()
