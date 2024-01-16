import argparse
import logging
import os
import sys
import numpy as np
import glob
import json
import threading
import time
import curses
from curses import wrapper
from collections import deque, defaultdict

def set_loglevel(
    loglevel,
    modulename=None, 
):
    logging.basicConfig(
        format="%(asctime)-5.5s %(name)-20.20s %(levelname)-7.7s %(message)s",
        datefmt="%H:%M",
        level=logging.WARNING,
    )

    logging.getLogger(__name__ if modulename is None else modulename).setLevel(loglevel)

def get_logger(
    name
):
    logging.basicConfig(
        format="%(asctime)-5.5s %(name)-20.20s %(levelname)-7.7s %(message)s",
        datefmt="%H:%M",
        level=logging.WARNING,
    )
    logging.getLogger(name).addHandler(logging.NullHandler())
    return logging.getLogger(name)

log = get_logger("event_monitor")

HEAD_TYPE = np.dtype([('tag', np.uint32), ('l1id', np.uint16), ('bcid', np.uint16), ('t_hits', np.uint16)])
DATA_TYPE = np.dtype([('col', np.uint16), ('row', np.uint16), ('tot', np.uint16)])

def read_header(file):
    return np.fromfile(file, dtype=HEAD_TYPE, count=1)

def read_data(file, hits):
    return np.fromfile(file, dtype=DATA_TYPE, count=hits)

def find_file_end(file, jump=0):
    file.seek(0, os.SEEK_END)
    return file.tell()

def find_chipsize(directory):
    scans = glob.glob(os.path.join(directory, '*Occupancy*.json'))
    if not scans:
        log.error('Cannot automatically infer chip sizes!')
        return 400,384
    jdata = json.load(open(scans[0], 'r'))
    
    # columns, rows
    return len(jdata['Data']), len(jdata['Data'][0])

from threading import Event

    
class buffer:
    def __init__(self, shape : tuple, dtype : np.dtype = int):
        self.data = np.empty(shape, dtype=dtype)
        self.maxitems = self.data.shape[0]
        self.idx : int =  0
        
    def fill(self, items: np.ndarray):
        if not isinstance(items, np.ndarray):
            items = np.array(items)
        if len(items.shape) > 1:
            assert items.shape[1] == self.data.shape[1]
        
        start = self.idx % self.maxitems
        end = start + len(items)
        if len(items) >= self.maxitems:
            self.data = items[len(items)-self.maxitems:]
        elif end > self.maxitems:
            self.data[start:] = items[:self.maxitems - start]
            self.data[:end - self.maxitems] = items[self.maxitems - start:]
        else:
            self.data[start:end] = items    
        
        self.idx += len(items)
        
    def get(self):
        return self.data[:min([self.idx, self.maxitems])]
    
    def clear(self):
        self.idx = 0
    
    def __repr__(self):
        return self.get().__repr__()
    
    def __str__(self):
        return self.get().__str__()

class dataThread(threading.Thread):
    __DATA_THREADS = 0
    __TIMING_BUFFERSIZE = 1_000_000
    def __init__(self, path, start=None, max_hits=500, find_end=False, *args, **kwargs):
        super().__init__(*args, **kwargs)
        
        self._exit = Event()
        if start < 0:
            start = None
        
        self._MAX_DATA_SIZE = max_hits
        self.data = buffer((self._MAX_DATA_SIZE, 3), dtype=int)
        self.header_counts = defaultdict(int)
        self.total_hits = 0
        self.total_events = 0
        
        self.n_events = 0
        self.aligned = False
        self.clear_flag = False
        
        # open file and manuever to correct start position
        self.path = path
        self.file = open(path, 'rb')
        self.find_end = find_end
                
        self.timeinfo = buffer((self.__TIMING_BUFFERSIZE, 3), float)
        dataThread.__DATA_THREADS += 1
        self.dt_number = self.__DATA_THREADS
        try:
            self.chip_name = os.path.basename(path).replace('_data.raw', '')
        except Exception as E:
            log.error(E)
            self.chip_name = "chip_{}".format(self.dt_number)
            
        self.log = get_logger('{} thr {}'.format(log.name, self.dt_number))
        set_loglevel(log.level, self.log.name)
        
        if start is None or start < 0:
            self.log.info("Finding EOF")
            idx = find_file_end(self.file)
            if start is not None:
                self.file.seek(start, 1)
                self.align()
        else:
            self.file.seek(start)
            self.align()
        
    
    def align(self, start_byte=b'\x9a\x02\x9a\x02'):
        # find our first start byte, or null
        start = self.file.tell()
        while not self.aligned:
            b = self.file.read()
            if not b:
                self.aligned = True
            if start_byte in b:
                self.file.seek(self.file.tell() - (len(b) - b.find(start_byte)) - 4)
                self.aligned = True
        
    def run(self):
        while not self._exit.is_set():
            if self.clear_flag:
                id0 = self.file.tell()
                if self.find_end:
                    idx = find_file_end(self.file)
                    self.log.debug("{} to {}".format(id0, idx))
                self.data.clear()
                self.n_events = 0
                self.clear_flag=False
            # continue on EOF
            idx = self.file.tell()
            header = read_header(self.file)
            if header is None or len(header)==0:
                self.timeinfo.fill([[time.time(), self.file.tell(), 1]])
                continue
            
            if not ((header['l1id'] == 666).all() and (header['bcid']==666).all()):                
                self.log.debug("seeking {} to {}".format(idx, idx - 10))
                self.timeinfo.fill([[time.time(), self.file.tell(), 2]])
                self.file.seek(-10, 1)
                n_hits = [1]
                # raise Exception("Failed at index: {} : {}".format(self.file.tell(), header))
            else:
                n_hits = header['t_hits']
            
            # if there is a header, see if it has events
            if sum(n_hits, start=0) == 0:
                # EOF
                self.header_counts[0] += 1
                self.timeinfo.fill([[time.time(), self.file.tell(), 3]])
                continue
            
            idx0 = self.file.tell()
            d = read_data(self.file, n_hits[0])
            self.file.seek(idx0 + n_hits[0]*6 - self.file.tell(), 1)
            self.header_counts[n_hits[0]] += 1
            self.log.debug("{}: {}".format(idx, d))
            self.data.fill(d.view(np.uint16).reshape(-1, 3).astype(int))
            self.total_hits += d.shape[0]
            self.n_events += 1
            self.total_events += 1
            self.timeinfo.fill([[time.time(), self.file.tell(), 0]])
            
        self.file.close()
        return 0
    
    def collect(self, clear=True):
        ret = self.data.get().copy(), (self.data.idx, self.data.maxitems, self.n_events)
        if clear:
            self.clear_flag = True
        else:
            self.clear_flag = False
        return ret
            
    def end_process(self):
        self.log.info("Exiting thread".format(self.dt_number))
        self._exit.set()

def debug_loop(threads, cols, rows):
    while True:
        try:
            time.sleep(.1)
            for thread in threads:
                thread.collect()

        except KeyboardInterrupt:
            return 0
        except Exception as E:
            log.error(E)
            return 1
        
def plot_timeinfo(threads):
    import matplotlib.pyplot as plt
    timeinfo = get_timeinfo(threads)
    for t,thread in zip(timeinfo,threads):
        plt.plot(t[:,0], t[:,1], marker='o', label=thread.chip_name, markersize=.2)
    plt.legend()
    plt.show()
    
def get_timeinfo(threads):
    res = []
    for thread in threads:
        t = thread.timeinfo.get()
        if len(t) > 0:
            t[:,0] -= t[0,0]
        res.append(t)
    return res

def measure_speed(thread, n_max=100000):
    t0 = time.time()
    last = thread.file.tell()
    tinfo = np.zeros((n_max, 2))
    
    if not thread.is_alive():
        thread.start()
    for i in range(n_max):         
        tinfo[i,:] = [time.time() - t0, thread.file.tell()]
        t = thread.file.tell()
        if last == t:
            break
        last = t
        
    return tinfo

def curses_loop(scr, threads, cols, rows, refresh_rate, n_memory=5):
    
    rowdiv, coldiv = 8, 4
    ymax, xmax = rows//rowdiv, cols//coldiv
    rowlen = 5
    box_start = 7 + 5*len(threads)
    lalign = 2
    
    curses.start_color()
    curses.use_default_colors()
    
    for i in range(0, curses.COLORS):
        curses.init_pair(i + 1, i, -1)
        
    scr.clear()
    info = curses.newwin(box_start - 1, 200, 1, lalign)
    
    outer = curses.newwin(ymax + 3, xmax + 5, box_start, lalign)
    outer.box()
    outer.refresh()
    win = curses.newwin(ymax + 1, xmax + 2, box_start + 1, lalign + 2)
    
    outersum = curses.newwin(ymax + 3, xmax + 5, box_start, lalign + xmax + 10)
    outersum.box()
    outersum.refresh()
    winsum = curses.newwin(ymax + 1, xmax + 2, box_start + 1, lalign + xmax + 12)
    
    #sum_values = np.linspace(0, x)
    # win.box()
    
    def draw(win, x, y, c, color=curses.color_pair(0)):
        try:
            win.addstr(y, x, c, color)
        except curses.error as e:
            pass

    k = 1
    x,y=None,None
    rcode = 0
    data = None
    curses.curs_set(0)
    n_complete = 0
    
    memory = [[None]*n_memory for i in range(len(threads))]
    sums = {}
    timing = buffer((30), float)
    
    time.sleep(1)
    t0 = time.time()
    
    while True:
        try:
            win.erase()
            info.erase()
            outer.box()
            outer.refresh()
            outersum.box()
            outersum.refresh()
            
            ntotal = 0
            for thread,char in zip(threads, range(len(threads))):
                data,meta = thread.collect(clear=True)
                
                memory[char][k % n_memory] = data
                combined = np.concatenate([cx for cx in memory[char] if cx is not None])

                for x,y in zip(combined[:,0], combined[:,1]):
                    draw(win, x//coldiv, y//rowdiv, str(char), curses.color_pair(char+2))
                for x,y in data[:,:2]:
                    draw(winsum, x//coldiv, y//rowdiv, str(char), curses.color_pair(char+2))
                    
                draw(info, 0, rowlen*char + 2, "Module {} ({})".format(char, thread.chip_name), curses.color_pair(char+2))
                draw(info, 3, rowlen*char + 3, "- Displayed hits: {}(/{} total in this cycle) (cumulative {})".format(combined.shape[0], meta[0], thread.total_hits))
                draw(info, 3, rowlen*char + 4, "- Events: {} (cumulative {})".format(meta[2], thread.total_events))
                draw(info, 3, rowlen*char + 5, "- File Position: {}".format(thread.file.tell()))
                
                # draw(info, 3, rowlen*char + 5, "- Headers: {}".format(dict(sorted(thread.header_counts.items()))))
                
                
                ntotal += data.shape[0]
            n_complete += ntotal
                
            if log.level == logging.DEBUG:
                draw(info, char + 5, 0, str(data))
            
            spf = timing.get()
            
            draw(info, 0, 0, "Frame: {} (real fps {:.1f})".format(k, 1/np.nanmean(spf) if len(spf) > 0 else 0))
            draw(info, 0, box_start-3, "Total Hits: {}".format(ntotal))
            draw(info, 0, box_start-2, "Cumulative Hits: {}".format(n_complete))
            
            # win.move(0, 0)        
            info.refresh()
            win.refresh()
            winsum.redrawwin()
            winsum.refresh()
            
            stime = max([0, 1/refresh_rate - (time.time() - t0)])
            time.sleep(stime)
            
            timing.fill([time.time() - t0])
            t0 = time.time()
            k += 1
            
        except KeyboardInterrupt as E:
            log.error("Keyboard interrupt. Exiting!")
            break
        except SystemExit as E:
            rcode = 2
            break
    win.clear()
    return rcode

def matplotlib_loop(threads, cols, rows, refresh_rate):
    import matplotlib.animation
    import matplotlib.pyplot as plt
    plt.ion()

    fig,ax = plt.subplots(1,1, figsize=(cols/100, rows/100))
    colors = plt.rcParams['axes.prop_cycle'].by_key()['color']
    scatters = [ax.scatter([], [], label=threads[i].chip_name, color=colors[i], marker='x') for i in range(len(threads))]
    plt.xlabel("Columns (x)")
    plt.ylabel("Rows (Y)")
    plt.xticks(np.arange(0, cols, 50))
    plt.xticks(np.arange(0, rows, 50))
    plt.grid()
    plt.legend(fontsize=9, frameon=False, loc='upper right') #, bbox_to_anchor=(1.0, 1.0))
    # plt.tight_layout()
    plt.axis('equal')
    plt.xlim(0, cols)
    plt.ylim(0, rows)
    
    rcode = 0
    t0 = time.time()
    while True:
        try:
            for thread,sc in zip(threads, scatters):
                data,meta = thread.collect(clear=True)    
                sc.set_offsets(np.c_[data[:,0], data[:,1]])
                fig.canvas.draw_idle()
            stime = max([0, 1/refresh_rate - (time.time() - t0)])
            plt.pause(stime)
            
            t0 = time.time()
        except KeyboardInterrupt as E:
            log.error("Keyboard interrupt. Exiting!")
            break
        except SystemExit as E:
            rcode = 2
            break
    
    return rcode
    
__CHOICE_MAP =  dict([(val, key) for key,values in [
    ("ascii", ["a", "asc"]), 
    ("matplotlib", ["mpl", "m"]), 
    ("interactive", ["i", "int"])
] for val in [key] + values])
    
if __name__=="__main__":
    p = argparse.ArgumentParser()
    p.add_argument('-d', '--debug', action='store_true', default=False, dest='debug', help='print low level logging msgs')
    p.add_argument('-r', '--refresh-rate', type=float, default=1.0, dest='refresh_rate', help='Max rate to refresh data reading')
    p.add_argument('--max-hits', default=1000, type=int, help='Maximum hits per frame')
    p.add_argument('--start', default=-1, type=int)
    p.add_argument('-m', '--mode', help="Choose visualization mode", choices=list(__CHOICE_MAP.keys()))
    p.add_argument('--no-seek', action='store_false', dest='seek_end', default=True, help='Do not seek file end upon reset')
    p.add_argument('DIRECTORY', help='Directory to store logging information in')
    p.add_argument('-s', '--sustain', help='Number of frames to keep events', default=1, type=int)
    
    args = p.parse_args()
    set_loglevel(logging.DEBUG if args.debug else logging.INFO, log.name)
    
    d = args.DIRECTORY
    if not os.path.exists(d): 
        log.fatal("Directory path {} does not exist!".format(d))
        sys.exit(1)
    
    paths = glob.glob(os.path.join(d, "*_data.raw"))
    if len(paths) == 0:
        log.fatal("No paths found for directory specification {}".format(d))
        sys.exit(1)
        
    cols, rows = find_chipsize(d)
    
    # create a data reading thread for each module
    threads = [dataThread(path, start=args.start, max_hits=args.max_hits, find_end=args.seek_end) for path in paths]
            
    rcode = 0
    if not (args.mode in __CHOICE_MAP or args.debug):
        log.info("")
        log.info("No loop selected!")
        log.info("")
    else:
        
        mode = "debug" if args.debug else __CHOICE_MAP[args.mode]

        # start threads
        for thread in threads:
            thread.start()

        if args.debug:
            rcode = debug_loop(threads, cols, rows)

        elif mode == "ascii":
            log.info("")
            log.info("Running curses loop..")
            log.info("")
            rcode = curses.wrapper(curses_loop, threads, cols, rows, args.refresh_rate, n_memory=args.sustain)
        elif mode == "matplotlib":
            log.info("")
            log.info("Running matplotlib loop")
            log.info("")
            rcode = matplotlib_loop(threads, cols, rows, args.refresh_rate)
        elif mode == "interactive":
            log.info("")
            log.info("Running interactive loop")
            log.info("")
            log.fatal("Interactive loop is not implemented! Ending process")
            
    for thread in threads:
        thread.end_process()
        thread.join()
        log.info("ended thread {}".format(str(thread)))          
        
    ti = get_timeinfo(threads)
    sys.exit(rcode)

