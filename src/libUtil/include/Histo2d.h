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
#include <typeinfo>
#include <typeindex>

#include "HistogramBase.h"
#include "ResultBase.h"

class Histo2d : public HistogramBase {
    public:
        Histo2d(const std::string &arg_name, unsigned arg_xbins, double arg_xlow, double arg_xhigh,
                unsigned arg_ybins, double arg_ylow, double arg_yhigh);
        Histo2d(const std::string &arg_name, unsigned arg_xbins, double arg_xlow, double arg_xhigh,
                unsigned arg_ybins, double arg_ylow, double arg_yhigh, const LoopStatus &stat);
        Histo2d(Histo2d *h);
        ~Histo2d() override;
        
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
        int binNum(double x, double y) const;
        
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
        double getMax() const {return max;}
        double getMin() const {return min;}
        double getNumOfEntries() const {return entries;}

        
        void toFile(const std::string &filename, const std::string &dir = "", bool header= true) const override;
        bool fromFile(const std::string &filename);
        void plot(const std::string &filename, const std::string &dir = "") const override;

    void toStream(std::ostream &out) const override;

    void toJson(json &j) const override;


private:
        std::vector<double> data;
        std::vector<bool> isFilled;

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

        //std::map<unsigned, bool> isFilled;
};

#endif
