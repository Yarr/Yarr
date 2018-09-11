#include "StdDataLoop.h"
#include "StdDataGatherer.h"
#include "StdDataAction.h"
#include "StdRepeater.h"

#ifndef ALLSTDACTIONS_H
#define ALLSTDACTIONS_H

#include "LoopActionBase.h"
#include <string>

namespace StdDict {
    bool registerLoopAction(std::string name,
                            std::function<std::unique_ptr<LoopActionBase>()> f);
    std::unique_ptr<LoopActionBase> getLoopAction(std::string name);

    std::vector<std::string> listLoopActions();
}

#endif
