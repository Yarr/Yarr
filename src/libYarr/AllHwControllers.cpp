#include "AllHwControllers.h"
#include "ClassRegistry.h"

#include <iostream>
#include <functional>

#include "logging.h"

namespace {
    auto ahwlog = logging::make_log("HwRegistry");
}

typedef ClassRegistry<HwController> OurRegistry;

static OurRegistry &registry() {
    static OurRegistry instance;
    return instance;
}

namespace StdDict {
    bool registerHwController(std::string name,
                              std::function<std::unique_ptr<HwController>()> f)
    {
        return registry().registerClass(name, f);
    }

    std::unique_ptr<HwController> getHwController(std::string name) {
        auto result = registry().makeClass(name);
        if(result == nullptr) {
            SPDLOG_LOGGER_ERROR(ahwlog, "No HwController matching '{}' found!", name);
        }
        return result;
    }

    std::vector<std::string> listHwControllers() {
        return registry().listClasses();
    }
}
