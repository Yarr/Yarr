/*
 * Original work:  T. Heim <timon.heim@cern.ch>,
 * Authors: Shohei Yamagaya <yamagaya@champ.hep.sci.osaka-u.ac.jp>
 * Date: 2019-Mar-12
 */

#include "Fei4GlobalFeedback.h"

void Fei4GlobalFeedback::writeConfig(json &config){
    config["min"] = min;
    config["max"] = max;
    config["step"] = step;
    config["parameter"] = parName;
}
void Fei4GlobalFeedback::loadConfig(json &config){
    if (config.contains("min"))
      min = config["min"];
    if (config.contains("max"))
      max = config["max"];
    if (config.contains("step"))
      step = config["step"];
    if (config.contains("parameter"))
      parName = config["parameter"];
}

