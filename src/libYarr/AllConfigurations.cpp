#include "AllConfigurations.h"
#include "ClassRegistry.h"

#include "logging.h"

namespace {
    auto aclog = logging::make_log("ConfigRegistry");

    typedef ClassRegistry<Configuration> OurRegistry;

    static OurRegistry &registry() {
        static OurRegistry instance;
        return instance;
    }
}

namespace StdDict {
    bool registerConfiguration(std::string name,
                               std::function<std::unique_ptr<Configuration>()> f)
    {
        return registry().registerClass(name, f);
    }

    std::unique_ptr<Configuration> getConfiguration(std::string name) {
        // If unspecified (for instance loadChipConfigs on commandline) use default
        if(name.empty()) name = "File";

        auto result = registry().makeClass(name);
        if(result == nullptr) {
            aclog->error("List available:");
            for(auto &s: listConfigurations()) {
                aclog->error(" {}", s);
            }
            SPDLOG_LOGGER_ERROR(aclog, "No Configuration backend matching '{}' found!", name);
        }
        return result;
    }

    std::vector<std::string> listConfigurations() {
        return registry().listClasses();
    }
}
