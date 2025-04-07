#ifndef YARR_ALL_CONFIGURES_H
#define YARR_ALL_CONFIGURES_H

#include "Configuration.h"

#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace StdDict {
    bool registerConfiguration(std::string name,
                               std::function<std::unique_ptr<Configuration>()> f);
    std::unique_ptr<Configuration> getConfiguration(std::string name);

    std::vector<std::string> listConfigurations();
}

#endif
