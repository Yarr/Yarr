#ifndef HISTO1D_H
#define HISTO1D_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: 1D Histogram
// # Comment: 
// ################################

#include <iostream>
#include <fstream>
#include <string>
#include <typeinfo>
#include <typeindex>
#include <cmath>

#include "HistogramBase.h"

class Histo1d : public HistogramBase {
    public:
        Histo1d(std::string arg_name, unsigned arg_bins, double arg_xlow, double arg_xhigh, std::type_index t);
        Histo1d(std::string arg_name, unsigned arg_bins, double arg_xlow, double arg_xhigh, std::type_index t, LoopStatus &stat);
        ~Histo1d();
        
        unsigned size() const;
        unsigned getEntries() const;
        double getMean();
        double getStdDev();

        void fill(double x, double v=1);

        void scale(const double s);
        void add(const Histo1d &h);
        
        void setBin(unsigned n, double v);
        double getBin(unsigned n) const;
        double* getData() { return data;};
        
        void toFile(std::string filename, std::string dir = "", bool header=true);
        void plot(std::string filename, std::string dir = "");

    private:
        double *data;
        double underflow;
        double overflow;

        unsigned bins;
        double xlow;
        double xhigh;
        double binWidth;

        double max;
        double min;
        unsigned entries;
        double sum;
};

#endif
