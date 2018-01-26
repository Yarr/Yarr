#include "AllStdActions.h"

static std::map<std::string, std::function<std::unique_ptr<LoopActionBase>()>> registry;

namespace AllStdActionsRegistry {
  using StdDict::registerLoopAction;

  bool data_loop_registered =
    registerLoopAction("StdDataLoop",
                       []() { return std::unique_ptr<LoopActionBase>(new StdDataLoop); });
}

namespace StdDict {
    bool registerLoopAction(std::string name,
                            std::function<std::unique_ptr<LoopActionBase>()> f)
    {
      registry[name] = f;
      return true;
    }

    std::unique_ptr<LoopActionBase> getLoopAction(std::string name) {
        try {
            return registry.at(name)();
        } catch(std::out_of_range &e) {
            std::cout << "No LoopAction matching '" << name << "' found\n";
            return nullptr;
        }
    }
}
