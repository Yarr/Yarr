#include "AllProcessors.h"
#include "ClassRegistry.h"

#include "logging.h"

namespace {
    auto aplog = logging::make_log("ProcRegistry");
}

typedef ClassRegistry<FeDataProcessor> OurRegistry;

static OurRegistry &registry() {
    static OurRegistry instance;
    return instance;
}

namespace StdDict {
    bool registerDataProcessor(std::string name,
                              std::function<std::unique_ptr<FeDataProcessor>()> f)
    {
        return registry().registerClass(name, f);
    }

    std::unique_ptr<FeDataProcessor> getDataProcessor(std::string name) {
        auto result = registry().makeClass(name);
        if(result == nullptr) {
            SPDLOG_LOGGER_ERROR(aplog, "No DataProcessor matching '{}' found!", name);
        }
        return result;
    }

    std::vector<std::string> listDataProcessors() {
        return registry().listClasses();
    }
}

