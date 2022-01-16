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
        int binNum(double x, double y, double z) const;
        
        double getUnderflow() const {return underflow;}
        double getOverflow() const {return overflow;}
        unsigned getXbins() const {return xbins;}
        double getXlow() const {return xlow;}
        double getXhigh() const {return xhigh;}
        double getXbinWidth() const {return xbinWidth;}
        unsigned getYbins() const {return ybins;}
        double getYlow() const {return ylow;}
        double getYhigh() const {return yhigh;}
        double getYbinWidth() const {return ybinWidth;}
        unsigned getZbins() const {return zbins;}
        double getZlow() const {return zlow;}
        double getZhigh() const {return zhigh;}
        double getZbinWidth() const {return zbinWidth;}
        double getMax() const {return max;}
        double getMin() const {return min;}
        double getNumOfEntries() const {return entries;}

        
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
