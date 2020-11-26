#ifndef HISTOGRAMBASE_H
#define HISTOGRAMBASE_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Histogram Base Container
// # Comment: 
// ################################

#include <string>

#include "LoopStatus.h"
#include "storage.hpp"

class HistogramBase {
    public:
        HistogramBase(const std::string &arg_name);
        HistogramBase(const std::string &arg_name, const LoopStatus &stat);
        virtual ~HistogramBase();

        const std::string & getName() const;

        const LoopStatus & getStat() const {return lStat;}
        virtual void toStream(std::ostream &out) const=0;
        virtual void toJson(json &j) const=0;
        virtual void toFile(const std::string &basename, const std::string &dir = "", bool header= true) const=0;
        virtual void plot(const std::string &basename, const std::string &dir = "") const=0;
        
        void setAxisTitle(std::string x, std::string y="y", std::string z="z");
        void setXaxisTitle(std::string);
        void setYaxisTitle(std::string);
        void setZaxisTitle(std::string);

        const std::string & getXaxisTitle() const;
        const std::string & getYaxisTitle() const;
        const std::string & getZaxisTitle() const;

    protected:
        std::string name;
        std::string xAxisTitle;
        std::string yAxisTitle;
        std::string zAxisTitle;
            
        LoopStatus lStat;
};
#endif
