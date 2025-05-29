#!/usr/bin/env python3
import sys
import json
import pyYARR


def test_config():
    con = pyYARR.ScanConsole()
    res = con.init(sys.argv)
    if res <= 0:
        return res
    res = con.loadConfig()
    if res != 0:
        return res
    config = con.getConfig()
    json_config = json.loads(config)
    newcon = pyYARR.ScanConsole()
    newcon.loadConfig(config)
    print(json.dumps(json_config))
    return 0

if __name__ == '__main__':
    test_config()
