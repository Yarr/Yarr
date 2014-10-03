// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: 2D Histogram
// # Comment: 
// ################################

#include "Histo2d.h"

Histo2d::Histo2d(std::string arg_name, unsigned arg_xbins, double arg_xlow, double arg_xhigh, 
        unsigned arg_ybins, double arg_ylow, double arg_yhigh) : ResultBase(arg_name) {
    xbins = arg_xbins;
    xlow = arg_xlow;
    xhigh = arg_xhigh;
    xbinWidth = (xhigh - xlow)/xbins;
    
    ybins = arg_ybins;
    ylow = arg_ylow;
    yhigh = arg_yhigh;
    ybinWidth = (yhigh - ylow)/ybins;
    
    min = 0;
    max = 0;
    data = new double[xbins*ybins];
}

Histo2d::~Histo2d() {
    delete data;
}

void Histo2d::fill(double x, double y, double v) {
    if (x < xlow || y < ylow) {
        underflow += v;
    } else if (x >= xhigh || y >= yhigh) {
        overflow += v;
    } else {
        unsigned xbin = (x-xlow)/xbinWidth;
        unsigned ybin = (y-ylow)/ybinWidth;
        data[xbin+(ybin*ybins)]+=v;
        if (v > max)
            max = v;
        if (v < min)
            min = v;
    }
}

void Histo2d::toFile(std::string filename) {
    std::fstream file(filename, std::fstream::out | std::fstream::trunc);
    // Header
    file << "Histo2d " << name << std::endl;
    file << xAxisTitle << " " << yAxisTitle << " " << zAxisTitle << std::endl;
    file << xbins << " " << xlow << " " << xhigh << std::endl;
    file << ybins << " " << ylow << " " << yhigh << std::endl;
    file << underflow << " " << overflow << std::endl;
    // Data
    for (unsigned int i=0; i<xbins; i++) {
        for (unsigned int j=0; j<ybins; j++) {
            file << data[i+(j*ybins)] << " ";
        }
        file << std::endl;
    }
}
