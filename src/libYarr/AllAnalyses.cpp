#include "AllAnalyses.h"
#include "ClassRegistry.h"

#include "logging.h"

namespace {
    auto aalog = logging::make_log("AnalysisRegistry");
}

typedef ClassRegistry<AnalysisAlgorithm> OurRegistry;

static OurRegistry &registry() {
    static OurRegistry instance;
    return instance;
}

namespace StdDict {
    bool registerAnalysis(std::string name,
                          std::function<std::unique_ptr<AnalysisAlgorithm>()> f)
    {
        return registry().registerClass(name, f);
    }

    std::unique_ptr<AnalysisAlgorithm> getAnalysis(std::string name) {
        auto result = registry().makeClass(name);

        if(result == nullptr) {
            SPDLOG_LOGGER_ERROR(aalog, "No AnalysisAlgorithm matching '{}' found!", name);
        }

        return result;
    }

    std::vector<std::string> listAnalyses() {
        return registry().listClasses();
    }
}

