// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Histogram Base Container
// # Comment: 
// ################################

#include "HistogramBase.h"

HistogramBase::HistogramBase(std::string arg_name, LoopStatus &stat) {
    name = arg_name;
    xAxisTitle = "x";
    yAxisTitle = "y";
    zAxisTitle = "z";
    lStat = stat;
}

HistogramBase::HistogramBase(std::string arg_name) {
    name = arg_name;
    xAxisTitle = "x";
    yAxisTitle = "y";
    zAxisTitle = "z";
}

HistogramBase::~HistogramBase() {

}

std::string HistogramBase::getName() {
    return name;
}

void HistogramBase::setAxisTitle(std::string x, std::string y, std::string z) {
    xAxisTitle = x;
    yAxisTitle = y;
    zAxisTitle = z;
}

std::string HistogramBase::getXaxisTitle() {
    return xAxisTitle;
}

std::string HistogramBase::getYaxisTitle() {
    return yAxisTitle;
}

std::string HistogramBase::getZaxisTitle() {
    return zAxisTitle;
}

void HistogramBase::setXaxisTitle(std::string name) {
    xAxisTitle = name;
}

void HistogramBase::setYaxisTitle(std::string name) {
    yAxisTitle = name;
}

void HistogramBase::setZaxisTitle(std::string name) {
    zAxisTitle = name;
}
