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

#include "ResultBase.h"

class Histo1d : public ResultBase {
    public:
        Histo1d(std::string arg_name, unsigned arg_bins, double arg_xlow, double arg_xhigh);
        ~Histo1d();
        
        unsigned size();

        void fill(double x, double v=1);
        
        void setBin(unsigned n, double v);
        double getBin(unsigned n);

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
};

#endif
