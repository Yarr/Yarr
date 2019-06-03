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
    if (!config["min"].empty())
      min = config["min"];
    if (!config["max"].empty())
      max = config["max"];
    if (!config["step"].empty())
      step = config["step"];
    if (!config["parameter"].empty())
      parName = config["parameter"];
}

