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

void ResultBase::setAxisTitle(std::string x, std::string y, std::string z) {
    xAxisTitle = std::move(x);
    yAxisTitle = std::move(y);
    zAxisTitle = std::move(z);
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

void ResultBase::setXaxisTitle(std::string name) {
    xAxisTitle = std::move(name);
}

void ResultBase::setYaxisTitle(std::string name) {
    yAxisTitle = std::move(name);
}

void ResultBase::setZaxisTitle(std::string name) {
    zAxisTitle = std::move(name);
}
