#ifndef ALLPROCESSORS_H
#define ALLPROCESSORS_H

#include "DataProcessor.h"

#include <iostream>
#include <string>
#include <vector>

namespace StdDict {
    bool registerDataProcessor(std::string name,
                              std::function<std::unique_ptr<DataProcessor>()> f);
    std::unique_ptr<DataProcessor> getDataProcessor(std::string name);

    std::vector<std::string> listDataProcessors();
}

#endif
