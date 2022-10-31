// #################################
// # Author: Timon Heim, magne.eik.laurizen at cern.ch and Simon Huiberts
// # Email: timon.heim at cern.ch, magne.eik.laurizen at cern.ch and simon.kristian.huiberts at cern.ch
// # Project: Yarr
// # Description: Two Parameter Loop for RD53A
// # Date: 03/2018
// ################################

#include "Rd53aTwoParameterLoop.h"

#include "logging.h"
#include <typeinfo>
#include <cstddef>

namespace {
  auto logger = logging::make_log("Rd53aTwoParameterLoop");
}

Rd53aTwoParameterLoop::Rd53aTwoParameterLoop() : LoopActionBase(LOOP_STYLE_PARAMETER) {
    loopType = typeid(this);
    min = 0;
    max = 100;
    step = 1;

}

Rd53aTwoParameterLoop::Rd53aTwoParameterLoop(Rd53Reg Rd53aGlobalCfg::*ref): LoopActionBase(LOOP_STYLE_PARAMETER), parPtr(ref) {
    loopType = typeid(this);
    min = 0;
    max = 100;
    step = 1;
    //TODO parName not set

}

void Rd53aTwoParameterLoop::init() {
    SPDLOG_LOGGER_TRACE(logger, "");
    m_done = false;

    for(std::vector<int>::size_type i = 0; i != m_cur.size(); i++){
          if(!multipleParams){
              m_cur[i] = min;
              parPtr = keeper->globalFe<Rd53a>()->regMap[parName];
          }
          else{
              m_cur[i] = minMultiple[i];
              parPtr = keeper->globalFe<Rd53a>()->regMap[parNameMultiple[i]];
          }
          this->writePar(parPtr, m_cur[i]);
      }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void Rd53aTwoParameterLoop::execPart1() {
  for(std::vector<int>::size_type i = 0; i != m_cur.size(); i++){
         std::string p;
         if(!multipleParams){p = parName;}
          else {p = parNameMultiple[i];}
          SPDLOG_LOGGER_TRACE(logger, "");
          logger->debug("ParameterLoop {} at -> {}", p, m_cur[i]);
        }
        g_stat->set(this, m_cur[m_curToLog] + add);
    //std::this_thread::sleep_for(std::chrono::milliseconds(10));
}




void Rd53aTwoParameterLoop::execPart2() {
  for(std::vector<int>::size_type i = 0; i != m_cur.size(); i++){
      if(!multipleParams){
          m_cur[i] += step;
          parPtr = keeper->globalFe<Rd53a>()->regMap[parName];
          if ((int)m_cur[i] > max) m_done = true;
      }else{
          m_cur[i] += stepMultiple[i];
          parPtr = keeper->globalFe<Rd53a>()->regMap[parNameMultiple[i]];
          if ((int)m_cur[i] > maxMultiple[i]) m_done = true;
      }
      this->writePar(parPtr, m_cur[i]);
  }
}

void Rd53aTwoParameterLoop::end() {
    // Reset to min
    for(std::vector<int>::size_type i = 0; i != m_cur.size(); i++){
         if(!multipleParams){
             m_cur[i] = min;
             parPtr = keeper->globalFe<Rd53a>()->regMap[parName];
         }else{
             m_cur[i] = minMultiple[i];
             parPtr = keeper->globalFe<Rd53a>()->regMap[parNameMultiple[i]];
         }
         this->writePar(parPtr, m_cur[i]);
     }

}

void Rd53aTwoParameterLoop::writePar(Rd53Reg Rd53aGlobalCfg::*p, uint32_t m) {
    keeper->globalFe<Rd53a>()->writeRegister(p, m);
    while(!g_tx->isCmdEmpty());
    //std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

void Rd53aTwoParameterLoop::writeConfig(json &j) {
    if(multipleParams){
      j["min"] = minMultiple[m_curToLog];
      j["max"] = maxMultiple[m_curToLog];
      j["step"] = stepMultiple[m_curToLog];
      j["parameter"] = parNameMultiple[m_curToLog];
    }else{
      j["min"] = min;
      j["max"] = max;
      j["step"] = step;
      j["parameter"] = parName;
    }
}

void Rd53aTwoParameterLoop::loadConfig(const json &j) {
    //Figure out if j contains dicts named 1, 2, 3, etc. each containing a min, max, step, and parname.
    //If an element named "1" is found, we assume multiple parameters to step has been given.
    //If not element named "1" is found, we assume only one parameter is to be stepped.
    json j_a;

    if ( j.find("1") == j.end() ) {
        j_a = j;
        multipleParams = false;
        //std::cout << "Processing single-parameter ParameterLoop." << std::endl;
    } else {
        j_a = j["1"];
        multipleParams = true;
        //std::cout << "Processing multi-parameter ParameterLoop." << std::endl;
    }
    if (j_a.contains("min")){
        min = j_a["min"];
    }
    if (j_a.contains("max")){
        max = j_a["max"];
    }
    if (j_a.contains("step")){
        step = j_a["step"];
    }
    if (j_a.contains("parameter")) {
        parName = j_a["parameter"];
    }


    if (multipleParams){

        for (auto it = j.begin(); it != j.end(); ++it){
              //json object j contains multiple types of objects:
              //1: Dictionaries named "1", "2", etc, each with min, max, step, and parameter.
              //2: Settings, namely "addValue" (int) and "log" (string).
              //We must only parse those objects which are json objects, i.e. _not_ one of the settings.
              j_a = it.value();
              if(j_a.size() == 4) {

              if (typeid(j_a) == typeid(json::object())){
                  if (j_a.contains("min")){
                      minMultiple.push_back(j_a["min"]);
                  }else{
                      minMultiple.push_back(0); //Default values are needed to keep all values (max, min, step, parameter) and the same index n in all the vectors.
                  }
                  if (j_a.contains("max")){
                      maxMultiple.push_back(j_a["max"]);
                  }else{
                      maxMultiple.push_back(0); //Default values are needed to keep all values (max, min, step, parameter) and the same index n in all the vectors.
                  }
                  if (j_a.contains("step")){
                      stepMultiple.push_back(j_a["step"]);
                  }else{
                      stepMultiple.push_back(0); //Default values are needed to keep all values (max, min, step, parameter) and the same index n in all the vectors.
                  }
                  if (j_a.contains("parameter")) {
                      parNameMultiple.push_back(j_a["parameter"]);
                  }else{
                      parNameMultiple.push_back(""); //Default values are needed to keep all values (max, min, step, parameter) and the same index n in all the vectors.
                  }
              }
        }
    }
}
    //We have a parameter which allows us to add a value to the logged value. This enables us to set offsets.
    if (j.contains("addValue")){
        add = j["addValue"];
    }
    //Parameter "log" lets the user decide which parameter to use for logging and analysis (e.g. g_stat.set()).
    //It is optional and defaults to the first parameter in the list of parameters to concurrently step.
    //If the parameter is not in the list of parameters to concurrently step it defaults to the first parameter.
    bool def = true; //Use default?
    if (j.contains("log")){
      parCompare = j["log"];
        for(std::vector<int>::size_type i = 0; i != parNameMultiple.size(); i++){
            if (parNameMultiple[i]== parCompare){
                std::cout << "  Logging parameter " << parNameMultiple[i] << std::endl;
                m_curToLog = i;
                def = false;
                min = minMultiple[i];
                max = maxMultiple[i];
                step = stepMultiple[i];
            }
        }
    }
    if (def){
        //"log" was either empty, or not found in the extra parameters to step.
        //Therefore, either the user specified to log the first parameter, or he/she gave a parameter that does not exist. In either way we log the first parameter.
        std::cout << "  Logging default parameter " << parName << std::endl;
        m_curToLog = 0;
    }
    if(multipleParams){
      m_cur.resize(parNameMultiple.size());
    }else{
      m_cur.resize(1);
    }
}
