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
    if (config.contains("min"))
      min = config["min"];
    if (config.contains("max"))
      max = config["max"];
    if (config.contains("step"))
      step = config["step"];
    if (config.contains("parameter"))
      parName = config["parameter"];
}

