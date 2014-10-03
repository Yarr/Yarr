// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: 1D Histogram
// # Comment: 
// ################################

#include "Histo1d.h"

Histo1d::Histo1d(std::string arg_name, unsigned arg_bins, double arg_xlow, double arg_xhigh) : ResultBase(arg_name) {
    bins = arg_bins;
    xlow = arg_xlow;
    xhigh = arg_xhigh;
    binWidth = (xhigh - xlow)/bins;
    data = new double[bins];
    for(unsigned i=0; i<bins; i++)
        data[i] = 0;
    min = 0;
    max = 0;
}

Histo1d::~Histo1d() {
    delete data;
}

unsigned Histo1d::size() {
    return bins;
}

void Histo1d::fill(double x, double v) {
    if (x < xlow) {
        underflow+=v;
    } else if (xhigh <= x) {
        overflow+=v;
    } else {
        unsigned bin = (x-xlow)/binWidth;
        data[bin]+=v;
        if (v > max)
            max = v;
        if (v < min)
            min = v;
    }
}

void Histo1d::setBin(unsigned n, double v) {
    if (n < bins) {
        data[n] = v;
        if (v > max)
            max = v;
        if (v < min)
            min = v;
    }
}

double Histo1d::getBin(unsigned n) {
    if (n < bins) {
        return data[n];
    }
    return 0;
}

