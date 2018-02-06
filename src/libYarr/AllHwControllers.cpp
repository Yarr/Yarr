#include "AllHwControllers.h"
#include "ClassRegistry.h"

typedef ClassRegistry<HwController> OurRegistry;

static OurRegistry &registry() {
    static OurRegistry instance;
    return instance;
}

namespace StdDict {
    bool registerHwController(std::string name,
                              std::function<std::unique_ptr<HwController>()> f)
    {
        return registry().registerClass(name, f);
    }

    std::unique_ptr<HwController> getHwController(std::string name) {
        auto result = registry().makeClass(name);
        if(result == nullptr) {
            std::cout << "No HwController matching '" << name << "' found\n";
        }
        return result;
    }

    std::vector<std::string> listHwControllers() {
        return registry().listClasses();
    }
}
