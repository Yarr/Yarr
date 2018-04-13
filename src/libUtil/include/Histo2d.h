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
#include <cmath>

#include "HistogramBase.h"
#include "ResultBase.h"

class Histo2d : public HistogramBase {
    public:
        Histo2d(std::string arg_name, unsigned arg_xbins, double arg_xlow, double arg_xhigh, 
                unsigned arg_ybins, double arg_ylow, double arg_yhigh, std::type_index t);
        Histo2d(std::string arg_name, unsigned arg_xbins, double arg_xlow, double arg_xhigh, 
                unsigned arg_ybins, double arg_ylow, double arg_yhigh, std::type_index t, LoopStatus &stat);
        Histo2d(Histo2d *h);
        ~Histo2d();
        
        unsigned size() const;
        unsigned numOfEntries() const;

        void fill(double x, double y, double v=1);
        void setAll(double v = 1);
        
        void add(const Histo2d &h);
        void subtract(const Histo2d &h);
        void multiply(const Histo2d &h);
        void divide(const Histo2d &h);
        void scale(const double s);
        void setBin(unsigned x, double v);

        double getMean();
        double getStdDev();
        
        double getBin(unsigned n) const;
        int binNum(double x, double y);
        
        double getUnderflow() {return underflow;}
        double getOverflow() {return overflow;}
        unsigned getXbins() {return xbins;}
        double getXlow() {return xlow;}
        double getXhigh() {return xhigh;}
        double getXbinWidth() {return xbinWidth;}
        unsigned getYbins() {return ybins;}
        double getYlow() {return ylow;}
        double getYhigh() {return yhigh;}
        double getYbinWidth() {return ybinWidth;}
        double getMax() {return max;}
        double getMin() {return min;}
        double getNumOfEntries() {return entries;}

        
        void toFile(std::string filename, std::string dir = "", bool header=true);
        void plot(std::string filename, std::string dir = "");

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
        unsigned entries;

        std::map<unsigned, bool> isFilled;
};

#endif
