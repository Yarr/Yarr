#include "AllChips.h"
#include "ClassRegistry.h"

#include "logging.h"

namespace {
    auto aclog = logging::make_log("ChipRegistry");
}

typedef ClassRegistry<FrontEnd> OurRegistry;

static OurRegistry &registry() {
    static OurRegistry instance;
    return instance;
}

namespace StdDict {
    bool registerFrontEnd(std::string name,
                              std::function<std::unique_ptr<FrontEnd>()> f)
    {
        return registry().registerClass(name, f);
    }

    std::unique_ptr<FrontEnd> getFrontEnd(std::string name) {
        auto result = registry().makeClass(name);
        if(result == nullptr) {
            aclog->error("List available:");
            for(auto &s: listFrontEnds()) {
                aclog->error(" {}", s);
            }
            SPDLOG_LOGGER_ERROR(aclog, "No FrontEnd matching '{}' found!", name);
        }
        return result;
    }

    std::vector<std::string> listFrontEnds() {
        return registry().listClasses();
    }
}

