/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-Oct-16
 */

#include "Fei4ParameterLoop.h"

void Fei4ParameterLoop::writeConfig(json &config){
    config["min"] = min;  
    config["max"] = max;
    config["step"] = step;
    config["parameter"] = parName;
}
void Fei4ParameterLoop::loadConfig(json &config){
    if (!config["min"].empty())
      min = config["min"];
    if (!config["max"].empty())
      max = config["max"];
    if (!config["step"].empty())
      step = config["step"];
    if (!config["parameter"].empty())
      parName = config["parameter"];
}

