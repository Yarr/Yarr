#include "AllChips.h"
#include "ClassRegistry.h"

typedef ClassRegistry<FrontEnd> OurRegistry;

static OurRegistry &registry() {
    static OurRegistry instance;
    return instance;
}

namespace StdDict {
    bool registerFrontEnd(std::string name,
                              std::function<std::unique_ptr<FrontEnd>()> f)
    {
        return registry().registerClass(name, f);
    }

    std::unique_ptr<FrontEnd> getFrontEnd(std::string name) {
        auto result = registry().makeClass(name);
        if(result == nullptr) {
            std::cout << "No FrontEnd matching '" << name << "' found\n";
        }
        return result;
    }

    std::vector<std::string> listFrontEnds() {
        return registry().listClasses();
    }
}

