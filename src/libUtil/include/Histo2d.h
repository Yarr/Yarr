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
#include <typeinfo>
#include <typeindex>
#include <stdio.h>

#include "HistogramBase.h"
#include "ResultBase.h"

class Histo2d : public HistogramBase {
    public:
        Histo2d(std::string arg_name, unsigned arg_xbins, double arg_xlow, double arg_xhigh, 
                unsigned arg_ybins, double arg_ylow, double arg_yhigh, std::type_index t);
        Histo2d(std::string arg_name, unsigned arg_xbins, double arg_xlow, double arg_xhigh, 
                unsigned arg_ybins, double arg_ylow, double arg_yhigh, std::type_index t, LoopStatus &stat);
        ~Histo2d();
        
        unsigned size() const;

        void fill(double x, double y, double v=1);
        void setAll(double v = 1);
        
        void add(const Histo2d &h);
        void subtract(const Histo2d &h);
        void multiply(const Histo2d &h);
        void divide(const Histo2d &h);
        //void setBin(unsigned x, double v);
        
        double getBin(unsigned n) const;
        
        void toFile(std::string filename, bool header=true);
        void plot(std::string filename);

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
