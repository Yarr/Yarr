#ifndef HISTO3D_H
#define HISTO3D_H

// #################################
// # Author: Eunchong 
// # Email: eunchong at cern.ch
// # Project: Yarr
// # Description: 3D Histogram
// # Comment: 
// ################################

#include <string>
#include <typeinfo>
#include <typeindex>

#include "HistogramBase.h"
#include "ResultBase.h"

class Histo3d : public HistogramBase {
    public:
        Histo3d(const std::string &arg_name, unsigned arg_xbins, double arg_xlow, double arg_xhigh,
                unsigned arg_ybins, double arg_ylow, double arg_yhigh,
                unsigned arg_zbins, double arg_zlow, double arg_zhigh);
        Histo3d(std::string arg_name, unsigned arg_xbins, double arg_xlow, double arg_xhigh, 
                unsigned arg_ybins, double arg_ylow, double arg_yhigh, 
                unsigned arg_zbins, double arg_zlow, double arg_zhigh, 
                const LoopStatus &stat);
        Histo3d(Histo3d *h);
        ~Histo3d() override;
        
        unsigned size() const;
        unsigned numOfEntries() const;

        void fill(double x, double y, double z, double v=1);
        void setAll(double v = 1);
        
        void add(const Histo3d &h);
        void subtract(const Histo3d &h);
        void multiply(const Histo3d &h);
        void divide(const Histo3d &h);
        void scale(const double s);
        void setBin(unsigned n, double v);

        double getMean();
        double getStdDev();
        
        double getBin(unsigned n) const;
        int binNum(double x, double y, double z);
        
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
        unsigned getZbins() {return zbins;}
        double getZlow() {return zlow;}
        double getZhigh() {return zhigh;}
        double getZbinWidth() {return zbinWidth;}
        double getMax() {return max;}
        double getMin() {return min;}
        double getNumOfEntries() {return entries;}

        
        void toFile(const std::string &filename, const std::string &dir = "", bool header= true) const override;
        bool fromFile(const std::string &filename);
        void plot(const std::string &filename, const std::string &dir = "") const override;

    void toStream(std::ostream &out) const override;

    void toJson(json &j) const override;

private:
        std::vector<uint16_t > data;

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

        unsigned zbins;
        double zlow;
        double zhigh;
        double zbinWidth;

        double max;
        double min;
        unsigned entries;

        std::map<unsigned, bool> isFilled;
};

#endif
