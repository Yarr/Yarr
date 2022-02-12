#! /usr/bin/python
import argparse
import math
import json

# Main function
if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--input', '-i', type=str, required=True, help="Input file")
    parser.add_argument('--output', '-o', type=str, required=True, help="Output file") 

    parser.add_argument('--th1', type=int, required=False, default=-1, help="TH1 DAC value")
    parser.add_argument('--th2', type=int, required=False, default=-1, help="TH2 DAC value")
    parser.add_argument('--tdac', type=int, required=False, default=99, help="TDAC value")

    parser.add_argument('--diffPreComp', type=int, required=False, default=300, help="Differential pre-comparator DAC value")
    parser.add_argument('--diffComp', type=int, required=False, default=500, help="Differential comparator DAC value")
    parser.add_argument('--diffVff', type=int, required=False, default=150, help="Preamp feedback DAC value")
    parser.add_argument('--diffPreamp', type=int, required=False, default=900, help="Preamp bias DAC value")
    parser.add_argument('--diffLccEn', type=int, required=False, default=0, help="LCC toggle")

    args = parser.parse_args()
    
    with open(args.input) as infile:
        data = json.load(infile)
    infile.close()
    
    data["RD53B"]["GlobalConfig"]["DiffPreampL"]=args.diffPreamp
    data["RD53B"]["GlobalConfig"]["DiffPreampM"]=args.diffPreamp
    data["RD53B"]["GlobalConfig"]["DiffPreampR"]=args.diffPreamp
    data["RD53B"]["GlobalConfig"]["DiffPreampT"]=args.diffPreamp
    data["RD53B"]["GlobalConfig"]["DiffPreampTL"]=args.diffPreamp
    data["RD53B"]["GlobalConfig"]["DiffPreampTR"]=args.diffPreamp
    data["RD53B"]["GlobalConfig"]["DiffVff"]=args.diffVff
    data["RD53B"]["GlobalConfig"]["DiffPreComp"]=args.diffPreComp
    data["RD53B"]["GlobalConfig"]["DiffComp"]=args.diffComp
    data["RD53B"]["GlobalConfig"]["DiffLccEn"]=args.diffLccEn

    if args.th1 >= 0:
        data["RD53B"]["GlobalConfig"]["DiffTh1L"]=args.th1
        data["RD53B"]["GlobalConfig"]["DiffTh1M"]=args.th1
        data["RD53B"]["GlobalConfig"]["DiffTh1R"]=args.th1

    if args.th2 >= 0:
        data["RD53B"]["GlobalConfig"]["DiffTh2"]=args.th2
    
    if math.fabs(args.tdac) < 16:
        for icol in range(len(data["RD53B"]["PixelConfig"])):
            for irow in range(len(data["RD53B"]["PixelConfig"][icol]["TDAC"])):
                data["RD53B"]["PixelConfig"][icol]["TDAC"][irow]=args.tdac                    

    with open(args.output, 'w') as outfile:
        outfile.write(json.dumps(data, sort_keys=True, indent=4))
    outfile.close()
