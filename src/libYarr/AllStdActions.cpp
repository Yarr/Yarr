#include "AllStdActions.h"
#include "ClassRegistry.h"

#include "logging.h"

namespace {
    auto asalog = logging::make_log("StdRegistry");
}

#include <functional>
typedef ClassRegistry<LoopActionBase> OurRegistry;

static OurRegistry &registry() {
  static OurRegistry instance;
  return instance;
}

namespace AllStdActionsRegistry {
  using StdDict::registerLoopAction;

  bool data_loop_registered =
    registerLoopAction("StdDataLoop",
                       []() { return std::unique_ptr<LoopActionBase>(new StdDataLoop); });

  bool data_gatherer_registered =
    registerLoopAction("StdDataGatherer",
                       []() { return std::unique_ptr<LoopActionBase>(new StdDataGatherer); });

  bool repeater_registered =
    registerLoopAction("StdRepeater",
                       []() { return std::unique_ptr<LoopActionBase>(new StdRepeater); });

  bool param_loop_registered =
    registerLoopAction("StdParameterLoop",
                       []() { return std::unique_ptr<LoopActionBase>(new StdParameterLoop); });
}

namespace StdDict {
    bool registerLoopAction(std::string name,
                            std::function<std::unique_ptr<LoopActionBase>()> f)
    {
        return registry().registerClass(name, f);
    }

    std::unique_ptr<LoopActionBase> getLoopAction(std::string name) {
        auto result = registry().makeClass(name);
        if(result == nullptr) {
            SPDLOG_LOGGER_ERROR(asalog, "No LoopAction matching '{}' found!", name);
        }
        return result;
    }

    std::vector<std::string> listLoopActions() {
        return registry().listClasses();
    }
}
