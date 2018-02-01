#include "AllHwControllers.h"

typedef std::map<std::string, std::function<std::unique_ptr<HwController>()>> MapType;

static MapType &registry() {
  static MapType instance;
  return instance;
}

namespace AllHwControllersRegistry {
  using StdDict::registerHwController;
}

namespace StdDict {
    bool registerHwController(std::string name,
                              std::function<std::unique_ptr<HwController>()> f)
    {
      registry()[name] = f;
      return true;
    }

    std::unique_ptr<HwController> getHwController(std::string name) {
        try {
            return registry().at(name)();
        } catch(std::out_of_range &e) {
            std::cout << "No HwController matching '" << name << "' found\n";
            return nullptr;
        }
    }

    std::vector<std::string> listHwControllers() {
        std::vector<std::string> known;
        for (auto &i: registry()) {
            known.push_back(i.first);
        }
        return known;
    }
}
