#include "AllHistogrammers.h"
#include "ClassRegistry.h"

#include "logging.h"

namespace {
    auto ahlog = logging::make_log("HistogammerRegistry");
}

typedef ClassRegistry<HistogramAlgorithm> OurRegistry;

static OurRegistry &registry() {
    static OurRegistry instance;
    return instance;
}

namespace StdDict {
    bool registerHistogrammer(std::string name,
                              std::function<std::unique_ptr<HistogramAlgorithm>()> f)
    {
        return registry().registerClass(name, f);
    }

    std::unique_ptr<HistogramAlgorithm> getHistogrammer(std::string name) {
        auto result = registry().makeClass(name);

        if(result == nullptr) { 
            SPDLOG_LOGGER_ERROR(ahlog, "No Histogrammer matching '{}' found!", name);
        }

        return result;
    }

    std::vector<std::string> listHistogrammers() {
        return registry().listClasses();
    }
}

