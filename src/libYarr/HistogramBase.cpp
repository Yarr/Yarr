// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Histogram Base Container
// # Comment: 
// ################################

#include "HistogramBase.h"

HistogramBase::HistogramBase(const std::string &arg_name, const LoopStatus &stat)
  : lStat(stat) {
    name = arg_name;
    xAxisTitle = "x";
    yAxisTitle = "y";
    zAxisTitle = "z";
}

HistogramBase::HistogramBase(const std::string &arg_name)
  : lStat(std::move(LoopStatus::empty())) ,name(std::move(arg_name)){
    xAxisTitle = "x";
    yAxisTitle = "y";
    zAxisTitle = "z";
}

HistogramBase::~HistogramBase() = default;

const std::string & HistogramBase::getName() const{
    return name;
}

void HistogramBase::setAxisTitle(std::string x, std::string y, std::string z) {
    xAxisTitle = x;
    yAxisTitle = y;
    zAxisTitle = z;
}

const std::string & HistogramBase::getXaxisTitle() const {
    return xAxisTitle;
}

const std::string & HistogramBase::getYaxisTitle() const {
    return yAxisTitle;
}

const std::string & HistogramBase::getZaxisTitle() const {
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
