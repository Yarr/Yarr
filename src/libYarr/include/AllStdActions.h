#include "StdDataLoop.h"
#include "StdDataGatherer.h"

#ifndef ALLSTDACTIONS_H
#define ALLSTDACTIONS_H

#include "LoopActionBase.h"
#include <string>

namespace StdDict {
    LoopActionBase* getLoopAction(std::string name);
}

#endif
