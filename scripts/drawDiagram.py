#!/usr/bin/env python3

#####
# A script that draws the data pipeline diagram using ROOT from the JSON file "<output_dir>/diagram.json" produced by the scanConsole when running it with an argument "-g".
#
# Usage: python drawDiagram.py diagram.json [outputname]
# The default output name is "YarrDiagram.png" in case no outputname is specified.

import sys
import json
from array import array
from ROOT import TCanvas, TText, TBox, TPolyLine, TLine, gPad, TText

if len(sys.argv) == 2:
    fname = sys.argv[1]
    outname = "YarrDiagram.png"
elif len(sys.argv) == 3:
    fname = sys.argv[1]
    outname = sys.argv[2]
else:
    print("Usage: python drawDiagram.py diagram.json [outputname]")
    sys.exit()

# Open and read input json file
with open(fname, 'r') as fjson:
    diagram = json.load(fjson)

#print(diagram)

length = diagram["Length"]
width = diagram["Width"]

canvas = TCanvas("c","c", int(length*100), int(width*100))
gPad.Range(0, 0, length, width)

shapes = []
texts = []
lines = []

for i, node_d in enumerate(diagram["Diagram"]):

    if node_d["shape"] == "rectangle":
        x1 = node_d["x"][0]
        x2 = node_d["x"][1]
        y1 = node_d["y"][0]
        y2 = node_d["y"][1]
        shapes.append( TBox(x1, y1, x2, y2) )
        shapes[-1].Draw("l same")

        # cell length and width
        clen = x2 - x1
        cwid = y2 - y1

        # Add label
        texts.append( TText(x1+clen/4, (y1+y2)/2., node_d["label"]) )
        texts[-1].SetTextSize(0.01)
        texts[-1].Draw("same")

        # Add left label
        if "label_left" in node_d:
            lsleft = node_d["label_left"].split("\n")
            for l in range(len(lsleft)):
                xleft = x1-clen/5.
                yleft = y2 - cwid/4. - cwid/2.*l
                texts.append( TText(xleft, yleft, lsleft[l]) )
                texts[-1].SetTextSize(0.008)
                texts[-1].Draw("same")

    elif node_d["shape"] == "hexagon":
        xarr = array('f', node_d["x"])
        yarr = array('f', node_d["y"])
        shapes.append( TPolyLine(len(xarr), xarr, yarr) )
        shapes[-1].Draw("same")

        # Add label
        x1 = min(xarr)
        x2 = max(xarr)
        y1 = min(yarr)
        y2 = max(yarr)
        texts.append( TText(x1, (y1+y2)/2., node_d["label"]) )
        texts[-1].SetTextSize(0.01)
        texts[-1].Draw("same")

        # cell length and width
        clen = x2 - x1
        cwid = y2 - y1

        if "label_left" in node_d:
            texts.append( TText(x1 - clen/10., y2 - cwid/4., node_d["label_left"]) )
            texts[-1].SetTextSize(0.008)
            texts[-1].Draw("same")

        if "label_right" in node_d:
            texts.append( TText(x2, y2 - cwid/4., node_d["label_right"]) )
            texts[-1].SetTextSize(0.008)
            texts[-1].Draw("samee")

    elif node_d["shape"] == "line":
        x1 = node_d["x"][0]
        x2 = node_d["x"][1]
        y1 = node_d["y"][0]
        y2 = node_d["y"][1]
        lines.append( TLine(x1, y1, x2, y2) )
        lines[-1].Draw("same")
    else:
        print(f"ERROR: unknow shape {node_d['shape']}")

canvas.Print(outname)
