'''
Script to plot CmlBias scan results 

Author: Maria Mironova (maria.mironova@cern.ch)

usage: plot_cmlbias.py [-h] [-r RESULTS_DIRECTORY] [-l N_LANES]

optional arguments:
  -h, --help            show this help message and exit
  -r RESULTS_DIRECTORY, --results_directory RESULTS_DIRECTORY
                        Directory with the CmlBias Scan results, default is
                        cmlbias_results
  -l N_LANES, --n_lanes N_LANES
                        Number of lanes, default is 4
'''

import matplotlib.pyplot as plt
from matplotlib import gridspec
import json 
import re, os, sys
import argparse
from plot_eyediagram import plot_eyediagram


def plot_cmlbias(directory,n_lanes):
	cmlbiases0 = set()
	cmlbiases1 = set()

	pattern = r'results_(\d+)_(\d+)\.txt'

	for filename in os.listdir(directory):
		match = re.match(pattern, filename)
		if match:
			cmlbiases0.add(int(match.group(1)))
			cmlbiases1.add(int(match.group(2)))

	cmlbiases0=sorted(list(cmlbiases0))
	cmlbiases1=sorted(list(cmlbiases1), reverse=True)
	n_cols=len(cmlbiases0)
	n_rows=len(cmlbiases1)

	fig = plt.figure(figsize=(14,n_lanes+16/n_lanes))
	spec = gridspec.GridSpec(ncols=n_cols, nrows=n_rows)

	for i in range(0, len(cmlbiases1)):
		for j in range(0, len(cmlbiases0)):
			if cmlbiases1[i]>=cmlbiases0[j]:
				if n_cols*i+j==n_cols:
					ax = fig.add_subplot(spec[n_cols*i+j])
					plt.axis('off')
			else: 			
				ax = fig.add_subplot(spec[n_cols*i+j])
				ax,im,data,labels = plot_eyediagram("%s/results_%s_%s.txt"%(directory,cmlbiases0[j],cmlbiases1[i]),n_lanes,ax)
				for m in range(n_lanes):
					for k, value in enumerate(data["0"]):
						text = ax.text(k, m, labels[m][k], ha="center", va="center", color="black",fontsize=5)
				ax.set_title("CmlBias0 %s CmlBias1 %s" %(cmlbiases0[j],cmlbiases1[i]), fontsize=9)
				ax.set_xticks([0,10,20,30])
				ax.set_xticklabels([0,10,20,30],fontsize=8)
				ax.set_yticklabels(ax.get_yticklabels(), fontsize=8)
				ax.set_xlabel(ax.get_xlabel(),fontsize=9)
				cbar = fig.colorbar(im, ax=ax)
				cbar.ax.tick_params(labelsize=9)  
	plt.yticks(rotation=0) 
	plt.tight_layout()
	plt.savefig("CmlBiasScan.png",dpi=200)
	plt.show()



if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-r', '--results_directory', default="cmlbias_results", help="Directory with the CmlBias Scan results, default is cmlbias_results")
    parser.add_argument('-l', '--n_lanes', default=4, help="Number of lanes, default is 4")
    args = vars(parser.parse_args())

    directory = args["results_directory"]
    n_lanes = int(args["n_lanes"])

    plot_cmlbias(directory,n_lanes)