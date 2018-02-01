#include "AllStdActions.h"

typedef std::map<std::string, std::function<std::unique_ptr<LoopActionBase>()>> MapType;

static MapType &registry() {
  static MapType instance;
  return instance;
}

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
      registry()[name] = f;
      return true;
    }

    std::unique_ptr<LoopActionBase> getLoopAction(std::string name) {
        try {
            return registry().at(name)();
        } catch(std::out_of_range &e) {
            std::cout << "No LoopAction matching '" << name << "' found\n";
            return nullptr;
        }
    }

    std::vector<std::string> listLoopActions() {
        std::vector<std::string> known;
        for (auto &i: registry()) {
            known.push_back(i.first);
        }
        return known;
    }
}
