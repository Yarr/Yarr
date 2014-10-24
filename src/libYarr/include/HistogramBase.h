#ifndef HISTOGRAMBASE_H
#define HISTOGRAMBASE_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Histogram Base Container
// # Comment: 
// ################################

#include <iostream>
#include <string>

#include "LoopStatus.h"

class HistogramBase {
    public:
        HistogramBase(std::string arg_name);
        HistogramBase(std::string arg_name, LoopStatus &stat);
        virtual ~HistogramBase();

        std::string getName();

        virtual void toFile(std::string basename, bool header=true) {}
        virtual void plot(std::string basename) {}
        
        void setAxisTitle(std::string x, std::string y="y", std::string z="z");
        void setXaxisTitle(std::string);
        void setYaxisTitle(std::string);
        void setZaxisTitle(std::string);

        std::string getXaxisTitle();
        std::string getYaxisTitle();
        std::string getZaxisTitle();

    protected:
        std::string name;
        std::string xAxisTitle;
        std::string yAxisTitle;
        std::string zAxisTitle;
            
        LoopStatus lStat;
    private:
};
#endif
