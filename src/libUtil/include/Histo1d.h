#ifndef HISTO1D_H
#define HISTO1D_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: 1D Histogram
// # Comment: 
// ################################

#include <string>
#include <typeinfo>
#include <typeindex>

#include "HistogramBase.h"

class Histo1d : public HistogramBase {
    public:
        Histo1d(const std::string &arg_name, unsigned arg_bins, double arg_xlow, double arg_xhigh);
        Histo1d(const std::string &arg_name, unsigned arg_bins, double arg_xlow, double arg_xhigh, const LoopStatus &stat);
        ~Histo1d() override;
        
        unsigned size() const;
        unsigned getEntries() const;
        double getMean();
        double getStdDev();

        void fill(double x, double v=1);

        void scale(const double s);
        void add(const Histo1d &h);
        
        void setBin(unsigned n, double v);
        double getBin(unsigned n) const;
        double const * getData() const { return data.data();};
        double getUnderflow() const {return underflow;};
        double getOverflow() const {return overflow;};
        
        void toFile(const std::string &filename, const std::string &dir = "", bool header= true) const override;
        bool fromFile(const std::string &filename);
        void plot(const std::string &filename, const std::string &dir = "") const override;

    void toStream(std::ostream &out) const override;

    void toJson(json &j) const override;

private:
        std::vector<double> data;
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
