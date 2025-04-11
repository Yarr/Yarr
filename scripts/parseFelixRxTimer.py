#!/usr/bin/env python3
######
# A script to analyze timer logs from FelixRxCore to benchmark its performance
######
import os
import sys
import csv
import pandas as pd
import matplotlib.pyplot as plt

if len(sys.argv) < 2:
    print("Usage: python parseFelixRxTimer.py <felix_rx_timer_log.csv> [outputdir]")
    sys.exit(1)
else:
    logfilename = sys.argv[1]
    outputdir = sys.argv[2] if len(sys.argv)>2 else os.path.splitext(logfilename)[0]
    if not os.path.isdir(outputdir):
        os.makedirs(outputdir)

# statistics
class Statistics:
    def __init__(self):
        self.start_time = 0
        self.seconds = 0
        self.packets = 0
        self.bytes = 0

    def start_timer(self, start_time):
        self.start_time = start_time

    def stop_timer(self, stop_time):
        assert stop_time >= self.start_time
        assert self.start_time > 0
        self.seconds += stop_time - self.start_time
        self.start_time = stop_time

    def add_packet(self, nbtyes):
        self.packets += 1
        self.bytes += nbtyes

def parse_timestamp(timestamp_str):
    # Minutes:Seconds.nanoseconds
    minutes, seconds = timestamp_str.split(":")
    minutes = int(minutes)
    seconds = float(seconds)
    return 60*minutes + seconds

def time_proc(id_key, stats_d, logentry):
    timestamp = parse_timestamp(logentry['timestamp'])
    start = logentry['start_or_done']=='start'
    key = logentry[id_key]
    nbytes = int(logentry['bytes'])

    if not key in stats_d: # new id
        stats_d[key] = Statistics()

    if start:
        stats_d[key].start_timer(timestamp)
    else:
        stats_d[key].stop_timer(timestamp)
        stats_d[key].add_packet(nbytes)

    return stats_d

def time_input(id_key, stats_d, logentry):
    timestamp = parse_timestamp(logentry['timestamp'])
    start = logentry['start_or_done']=='start'
    key = logentry[id_key]
    nbytes = int(logentry['bytes'])

    # only count either start or stop
    if not start:
        return stats_d

    if not key in stats_d: # new id
        stats_d[key] = Statistics()
        stats_d[key].start_timer(timestamp)
    else:
        stats_d[key].stop_timer(timestamp)
        stats_d[key].add_packet(nbytes)

    return stats_d

fid_stats_input = {}
thread_stats_input = {}

fid_stats_proc = {}
thread_stats_proc = {}

thread_fid_map = {}

with open(logfilename, newline='') as f:
    reader = csv.DictReader(f)

    # overwrite the first field name
    reader.fieldnames[0] = 'timestamp'
    # field names: 'timestamp', 'function', 'start_or_done', 'thread', 'fid', 'bytes'

    for entry in reader:
        funcname = entry['function']

        # per fid
        if not funcname in fid_stats_proc: # new function
            fid_stats_proc[funcname] = {}

        time_proc('fid', fid_stats_proc[funcname], entry)

        if not funcname in fid_stats_input: # new function
            fid_stats_input[funcname] = {}

        time_input('fid', fid_stats_input[funcname], entry)

        # per thread
        if not funcname in thread_stats_proc: # new function
            thread_stats_proc[funcname] = {}

        time_proc('thread', thread_stats_proc[funcname], entry)

        if not funcname in thread_stats_input: # new function
            thread_stats_input[funcname] = {}

        time_input('thread', thread_stats_input[funcname], entry)

        # thread fid map
        if not funcname in thread_fid_map:
            thread_fid_map[funcname] = {}

        if not entry['thread'] in thread_fid_map[funcname]:
            thread_fid_map[funcname][ entry['thread'] ] =  set()

        thread_fid_map[funcname][ entry['thread'] ].add( entry['fid'] )

# Compute benchmarks
def compute_benchmarks(stats_d, keyname):
    benchmarks = []
    index = []
    for key, stats in stats_d.items():
        index.append(key)
        benchmarks.append({
            "Time[seconds]" : stats.seconds,
            "Packets" : stats.packets,
            "Bytes" : stats.bytes,
            "Rate[kHz]" : stats.packets/stats.seconds/1e3,
            "Throughput[kB/s]" : stats.bytes/stats.seconds/1e3
        })

    df_bm = pd.DataFrame(benchmarks, index=index)
    df_bm.index.name = keyname

    return df_bm

for funcname in thread_fid_map:
    print("------------")
    print(f"{funcname}:")
    for thread in thread_fid_map[funcname]:
        fids = list(thread_fid_map[funcname][thread])
        fids.sort()
        print(f"thread id: {thread}")
        for fid in fids:
            print(f"- fid: {fid}")

functions = []
benchmarks_df_d = {
    'thread': {'input': [], 'proc': []},
    'fid': {'input': [], 'proc': []},
}

for funcname in thread_fid_map:
    #print("============")
    #print(funcname)
    functions.append(funcname)

    #outfilename = os.path.join(outputdir, funcname.replace("::","__"))

    benchmarks_df_d["thread"]["input"].append(
        compute_benchmarks(thread_stats_input[funcname], 'thread')
    )

    benchmarks_df_d["fid"]["input"].append(
        compute_benchmarks(fid_stats_input[funcname], 'fid')
    )

    benchmarks_df_d["thread"]['proc'].append(
        compute_benchmarks(thread_stats_proc[funcname], 'thread')
    )

    benchmarks_df_d["fid"]['proc'].append(
        compute_benchmarks(fid_stats_proc[funcname], 'fid')
    )

# merge the dataframes for different functions
# per thread
df_thread_input = pd.concat(benchmarks_df_d["thread"]["input"], keys=functions, names=['function'])
df_thread_proc = pd.concat(benchmarks_df_d["thread"]['proc'], keys=functions, names=['function'])
# per fid
df_fid_input = pd.concat(benchmarks_df_d["fid"]["input"], keys=functions, names=['function'])
df_fid_proc = pd.concat(benchmarks_df_d["fid"]["proc"], keys=functions, names=['function'])

print("---------------")
print("Input:")
print(df_thread_input)
print(df_fid_input)

print("---------------")
print("Processing:")
print(df_thread_proc)
print(df_fid_proc)

# plot
def plot_benchmark_df(benchmark_df, filename):

    funcname, keyname = benchmark_df.index.names

    for colname in list(benchmark_df):
        fieldname = colname.split('[')[0]

        unit = colname.split('[')[1:]
        unit = "["+unit[0] if unit else ''

        ax = benchmark_df.unstack(funcname).plot.barh(
            y=colname, 
            title=f"{fieldname} per {keyname}",
            xlabel = unit,
        )
        ax.bar_label(ax.containers[0])
        ax.bar_label(ax.containers[1])

        fig = ax.get_figure()
        fig.tight_layout()
        fig.savefig(filename+f"_{fieldname}.png")
        plt.close(fig)

plot_benchmark_df(df_thread_input, os.path.join(outputdir, "input_per_thread"))
plot_benchmark_df(df_fid_input, os.path.join(outputdir, "input_per_fid"))
plot_benchmark_df(df_thread_proc, os.path.join(outputdir, "proc_per_thread"))
plot_benchmark_df(df_fid_proc, os.path.join(outputdir, "proc_per_fid"))