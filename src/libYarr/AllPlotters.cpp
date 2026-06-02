#include "AllPlotters.h"
#include "ClassRegistry.h"

#include "logging.h"

namespace {
    auto aplog = logging::make_log("PlottingRegistry");

    using OurRegistry = ClassRegistry<Plotter>;

    static OurRegistry &registry() {
        static OurRegistry instance;
        return instance;
    }
}

namespace StdDict {

bool registerPlotter(std::string name,
                     std::function<std::unique_ptr<Plotter>()> f)
{
    return registry().registerClass(std::move(name), std::move(f));
}

std::unique_ptr<Plotter> getPlotter(std::string name) {
    if(name.empty()) name = "Default";

    auto result = registry().makeClass(name);
    if(result == nullptr) {
        aplog->error("List available:");
        for(auto &s: listPlotters()) {
            aplog->error(" {}", s);
        }
        SPDLOG_LOGGER_ERROR(aplog, "No plotting backend matching '{}' found!", name);
    }
    return result;
}

std::vector<std::string> listPlotters() {
    return registry().listClasses();
}

} // End namespace
