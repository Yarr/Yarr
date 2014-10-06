// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: 2D Histogram
// # Comment: 
// ################################

#include "Histo2d.h"
#include <stdio.h>

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
        data[ybin+(xbin*ybins)]+=v;
        if (v > max)
            max = v;
        if (v < min)
            min = v;
    }
}

void Histo2d::toFile(std::string prefix, bool header) {
    std::string filename = prefix + "_" + ResultBase::name + ".dat";
    std::fstream file(filename, std::fstream::out | std::fstream::trunc);
    // Header
    if (header) {
        file << "Histo2d " << name << std::endl;
        file << xAxisTitle << " " << yAxisTitle << " " << zAxisTitle << std::endl;
        file << xbins << " " << xlow << " " << xhigh << std::endl;
        file << ybins << " " << ylow << " " << yhigh << std::endl;
        file << underflow << " " << overflow << std::endl;
    }
    // Data
    for (unsigned int i=0; i<ybins; i++) {
        for (unsigned int j=0; j<xbins; j++) {
            file << data[i+(j*ybins)] << " ";
        }
        file << std::endl;
    }
    file.close();
}

void Histo2d::plot(std::string prefix) {
    // Put raw histo data in tmp file
    this->toFile("/tmp/tmp", false);
    std::string cmd = "gnuplot | ps2pdf - " + prefix + "_" + ResultBase::name + ".pdf";

    // Open gnuplot as file and pipe commands
    FILE *gnu = popen(cmd.c_str(), "w");
    
    fprintf(gnu, "set terminal postscript enhanced color \"Helvetica\" 14\n");
    fprintf(gnu, "set palette defined ( 0 '#D53E4F', 1 '#F46D43', 2 '#FDAE61', 3 '#FEE08B', 4 '#E6F598', 5 '#ABDDA4', 6 '#66C2A5', 7 '#3288BD')\n");
    fprintf(gnu, "set pm3d map\n");
    fprintf(gnu, "unset key\n");
    fprintf(gnu, "set title \"%s\"\n" , ResultBase::name.c_str());
    fprintf(gnu, "set xlabel \"%s\"\n" , ResultBase::xAxisTitle.c_str());
    fprintf(gnu, "set ylabel \"%s\"\n" , ResultBase::yAxisTitle.c_str());
    fprintf(gnu, "set cblabel \"%s\"\n" , ResultBase::zAxisTitle.c_str());
    fprintf(gnu, "set xrange[%f:%f]\n", xlow, xhigh);
    fprintf(gnu, "set yrange[%f:%f]\n", ylow, yhigh);
    fprintf(gnu, "splot \"/tmp/tmp_%s.dat\" matrix u (($1+1)*((%f-%f)/%d)):(($2+1)*((%f-%f)/%d)):3\n", ResultBase::name.c_str(), xhigh, xlow, xbins, yhigh, ylow, ybins);
    pclose(gnu);
}

