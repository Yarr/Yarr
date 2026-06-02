// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Result Base Container
// # Comment: 
// ################################

#include "ResultBase.h"

ResultBase::ResultBase(std::string arg_name, LoopStatus &stat)
  : lStat(stat) {
    name = std::move(arg_name);
    xAxisTitle = "x";
    yAxisTitle = "y";
    zAxisTitle = "z";
}

ResultBase::ResultBase(std::string arg_name) {
    name = std::move(arg_name);
    xAxisTitle = "x";
    yAxisTitle = "y";
    zAxisTitle = "z";
}

ResultBase::~ResultBase() = default;

std::string ResultBase::getName() {
    return name;
}

void ResultBase::setAxisTitle(const std::string& x, const std::string& y, const std::string& z) {
    xAxisTitle = x;
    yAxisTitle = y;
    zAxisTitle = z;
}

std::string ResultBase::getXaxisTitle() {
    return xAxisTitle;
}

std::string ResultBase::getYaxisTitle() {
    return yAxisTitle;
}

std::string ResultBase::getZaxisTitle() {
    return zAxisTitle;
}

void ResultBase::setXaxisTitle(const std::string& name) {
    xAxisTitle = name;
}

void ResultBase::setYaxisTitle(const std::string& name) {
    yAxisTitle = name;
}

void ResultBase::setZaxisTitle(const std::string& name) {
    zAxisTitle = name;
}
