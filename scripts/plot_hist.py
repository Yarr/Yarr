"""
python script to plot single scan outputs like maps or distributions
author: Lingxin Meng (lmeng@cern.ch)
"""

import argparse
import json
import matplotlib.pyplot as plt
import numpy as np
import os
import logging
from rich.logging import RichHandler

handler = RichHandler(markup=True)
formatter = logging.Formatter(fmt="%(message)s", datefmt="[%X]")
handler.setFormatter(formatter)
logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)
logger.addHandler(handler)
logger.propagate = False

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--input', required=True, help="Input file.")
    parser.add_argument('-o', '--output', help="Output file name, can contain extension. Default output file format is png.")
    parser.add_argument('-s', '--save', action='store_true', help="Save the output plot? Default is False.")
    args = parser.parse_args()

    logger.info(args)

    if not os.path.isfile(args.input):
        logger.error("File doesn't exist!")
        exit()

    infile = open(args.input, "r")

    try:
        jason = json.load(infile)
    except Exception as e:
        logger.error(f"Can't read json file: {e}")
        exit()

    data = np.array(jason["Data"], dtype=float) ## 400 lists of 384 rows
    plottype = jason["Type"]
    xlow = jason["x"]["Low"]
    xhigh = jason["x"]["High"]
    xnbins = jason["x"]["Bins"]

    fig, ax = plt.subplots()

    if plottype == "Histo2d":
        im = ax.imshow(np.array(data).T.tolist(), aspect='auto', origin="lower") ## needs to be transposed, otherwise row and col are swapped
        cbar = ax.figure.colorbar(im, ax=ax)
        cbar.ax.set_ylabel(jason["z"]["AxisTitle"], rotation=-90, va="bottom")
    elif plottype == "Histo1d":
        step = (xhigh-xlow)/xnbins
        xdata = np.arange(xlow, xhigh, step)
        ax.bar(xdata, data)
        ax.set_xlim(xlow, xhigh)
    else:
        logger.error(f"I can't plot {plottype} yet, please implement :)")
        exit()

    ax.set_title(jason["Name"])
    ax.set_xlabel(jason["x"]["AxisTitle"])
    ax.set_ylabel(jason["y"]["AxisTitle"])

    fig.tight_layout()

    if args.save:
        if args.output:
            outname = args.output
            if not "/" in args.output:
                outdir = "/".join(args.input.split("/")[:-1])
                outname = "/".join([outdir,outname])
        else:
            outname = args.input.split(".")[0]

        if not "." in outname:
            logger.info("No output file extension provided, using default '.png'.")
            outname = ".".join([outname, "png"])

        logger.info(f"Saving {outname}.")
        fig.savefig(outname)

    else:
        plt.show()

    infile.close()
    plt.close()
    exit()
