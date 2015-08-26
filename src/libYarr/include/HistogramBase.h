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
#include <typeinfo>
#include <typeindex>

#include "LoopStatus.h"

class HistogramBase {
    public:
        HistogramBase(std::string arg_name, std::type_index t);
        HistogramBase(std::string arg_name, std::type_index t, LoopStatus &stat);
        virtual ~HistogramBase();

        std::string getName();

        LoopStatus getStat() {return lStat;}

        virtual void toFile(std::string basename, std::string dir = "", bool header=true) {}
        virtual void plot(std::string basename, std::string dir = "") {}
        
        void setAxisTitle(std::string x, std::string y="y", std::string z="z");
        void setXaxisTitle(std::string);
        void setYaxisTitle(std::string);
        void setZaxisTitle(std::string);

        std::string getXaxisTitle();
        std::string getYaxisTitle();
        std::string getZaxisTitle();

        std::type_index getType() {return type;}
    protected:
        std::string name;
        std::string xAxisTitle;
        std::string yAxisTitle;
        std::string zAxisTitle;
            
        LoopStatus lStat;
    private:
        std::type_index type;
};
#endif
