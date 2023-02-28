#!/usr/bin/env python3
import sys
import pyYARR
import json
import threading
import time

logConfig =  {
              "default_sink" : "ringbuffer",
              "ringbuffer_size" : 1000, 
              "pattern": "[%T:%e]%^[%=8l][%=15n]:%$ %v",
              "log_config": [ {"name": "all", "level" : "info"} ]
}
 

pyYARR.setupLogger(json.dumps(logConfig))
class ScanThread(threading.Thread):
    def __init__(self, args):
        super(ScanThread, self).__init__()
        self.args = args

    def run(self):
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
    thread = ScanThread(sys.argv)
    thread.start()
    poll = True
    while poll:
        lines = pyYARR.getLog(10)
        time.sleep(.1)
        if len(lines) == 0:
            continue
        for line in lines:
           print(line.rstrip())
           if "Finishing run:" in line:
               poll = False
               break
    print("Run ended")
    thread.join()

