// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Result Base Container
// # Comment: 
// ################################

#include "ResultBase.h"

ResultBase::ResultBase(std::string arg_name, LoopStatus &stat) {
    name = arg_name;
    xAxisTitle = "x";
    yAxisTitle = "y";
    zAxisTitle = "z";
    lStat = stat;
}

ResultBase::ResultBase(std::string arg_name) {
    name = arg_name;
    xAxisTitle = "x";
    yAxisTitle = "y";
    zAxisTitle = "z";
}

ResultBase::~ResultBase() {

}

std::string ResultBase::getName() {
    return name;
}

void ResultBase::setAxisTitle(std::string x, std::string y, std::string z) {
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

void ResultBase::setXaxisTitle(std::string name) {
    xAxisTitle = name;
}

void ResultBase::setYaxisTitle(std::string name) {
    yAxisTitle = name;
}

void ResultBase::setZaxisTitle(std::string name) {
    zAxisTitle = name;
}
