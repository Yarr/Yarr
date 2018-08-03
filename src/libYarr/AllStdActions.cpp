#include "AllStdActions.h"
#include "ClassRegistry.h"

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
            std::cout << "No LoopAction matching '" << name << "' found\n";
        }
        return result;
    }

    std::vector<std::string> listLoopActions() {
        return registry().listClasses();
    }
}
