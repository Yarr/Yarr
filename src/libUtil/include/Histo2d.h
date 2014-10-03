#ifndef HISTO2D_H
#define HISTO2D_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: 2D Histogram
// # Comment: 
// ################################

#include <string>
#include <fstream>

#include "ResultBase.h"

class Histo2d : ResultBase {
    public:
        Histo2d(std::string arg_name, unsigned arg_xbins, double arg_xlow, double arg_xhigh, 
                unsigned arg_ybins, double arg_ylow, double arg_yhigh);
        ~Histo2d();
        
        unsigned size();

        void fill(double x, double y, double v=1);
        
        //void setBin(unsigned x, double v);
        //double getBin(unsigned n);
        
        void toFile(std::string filename);

    private:
        double *data;

        double underflow;
        double overflow;

        unsigned xbins;
        double xlow;
        double xhigh;
        double xbinWidth;

        unsigned ybins;
        double ylow;
        double yhigh;
        double ybinWidth;

        double max;
        double min;
};

#endif
