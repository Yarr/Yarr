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

template<typename DataT>
class Histo3dT : public HistogramBase {
    public:
        Histo3dT(const std::string &arg_name, unsigned arg_xbins, double arg_xlow, double arg_xhigh,
                unsigned arg_ybins, double arg_ylow, double arg_yhigh,
                unsigned arg_zbins, double arg_zlow, double arg_zhigh);
        Histo3dT(std::string arg_name, unsigned arg_xbins, double arg_xlow, double arg_xhigh, 
                unsigned arg_ybins, double arg_ylow, double arg_yhigh, 
                unsigned arg_zbins, double arg_zlow, double arg_zhigh, 
                const LoopStatus &stat);
        Histo3dT(Histo3dT *h);
        ~Histo3dT() override;
        
        unsigned size() const;
        unsigned numOfEntries() const;

        void fill(double x, double y, double z, DataT v=1);
        void setAll(DataT v = 1);
        
        void add(const Histo3dT &h);
        void subtract(const Histo3dT &h);
        void multiply(const Histo3dT &h);
        void divide(const Histo3dT &h);
        void scale(const double s);
        void setBin(unsigned n, DataT v);

        double getMean() const;
        double getStdDev() const;
        
        double getBin(unsigned n) const;
        int binNum(double x, double y, double z) const;

	bool isFilled(unsigned n) const;
        
        DataT getUnderflow() const {return underflow;}
        DataT getOverflow() const {return overflow;}
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
        DataT getMax() const {return max;}
        DataT getMin() const {return min;}
        unsigned getNumOfEntries() const {return entries;}

        
        void toFile(const std::string &filename, const std::string &dir = "", bool header= true) const override;
        bool fromFile(const std::string &filename);
        bool fromJson(const json &jfile);
        void plot(const std::string &filename, const std::string &dir = "") const override;

        void toStream(std::ostream &out) const override;
        void toJson(json &j) const override;

private:
        std::vector<DataT> data;

        DataT underflow;
        DataT overflow;

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

        DataT max;
        DataT min;
        unsigned entries;

        std::vector<bool> m_isFilled;
};

using Histo3d = Histo3dT<uint16_t>;

#endif