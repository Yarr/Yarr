'''
Script to plot eye diagram results 
Author: Maria Mironova (maria.mironova@cern.ch)

usage: plot_eyediagram.py [-h] [-r RESULTS_FILE] [-l N_LANES]

optional arguments:
  -h, --help            show this help message and exit
  -r RESULTS_FILE, --results_file RESULTS_FILE
                        Eye diagram results file, default is results.txt
  -l N_LANES, --n_lanes N_LANES
                        Number of lanes, default is 4
'''


import matplotlib.pyplot as plt
import numpy as np 
import argparse

def plot_eyediagram(results,n_lanes,ax):
    lanes= [str(lane) for lane in range(0,n_lanes)]
    names= ["Delay"]+lanes
    data=np.genfromtxt('%s'%results, delimiter="|", names=names)

    labels = [["" if data[n][j] != 1 else "X" for j in range(len(data[n]))] for n in lanes]

    scale=n_lanes/4
    im = ax.imshow([data[lane] for lane in lanes],aspect='auto',vmin=0, vmax=1)

    ax.set_xticks(np.arange(len(data['0'])))
    ax.set_yticks(np.arange(len(lanes)))
    ax.set_yticklabels(labels=['Lane ' + lane for lane in lanes]) 
    ax.set_xlabel("Delay")
    return ax,im,data,labels



if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-r', '--results_file', default="results.txt", help="Eye diagram results file, default is results.txt")
    parser.add_argument('-l', '--n_lanes', default=4, help="Number of lanes, default is 4")
    args = vars(parser.parse_args())

    results = args["results_file"]
    n_lanes = int(args["n_lanes"])

    fig, ax = plt.subplots(figsize=(10,3*n_lanes/4))
    ax,im,data,labels=plot_eyediagram(results,n_lanes,ax)
    fig.colorbar(im, ax=ax, label="Link quality")
    for i in range(n_lanes):
        for j, value in enumerate(data["0"]):
            text = ax.text(j, i, labels[i][j], ha="center", va="center", color="black",fontsize=10)
    plt.tight_layout()
    plt.savefig("eye_diagram.png",dpi=200)
    plt.show()