#ifndef ALLPROCESSORS_H
#define ALLPROCESSORS_H

#include "FeDataProcessor.h"

#include <string>
#include <vector>
#include <functional>
namespace StdDict {
    bool registerDataProcessor(std::string name,
                              std::function<std::unique_ptr<FeDataProcessor>()> f);
    std::unique_ptr<FeDataProcessor> getDataProcessor(std::string name);

    std::vector<std::string> listDataProcessors();
}

#endif
