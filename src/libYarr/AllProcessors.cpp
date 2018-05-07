#include "AllProcessors.h"
#include "ClassRegistry.h"

typedef ClassRegistry<DataProcessor> OurRegistry;

static OurRegistry &registry() {
    static OurRegistry instance;
    return instance;
}

namespace StdDict {
    bool registerDataProcessor(std::string name,
                              std::function<std::unique_ptr<DataProcessor>()> f)
    {
        return registry().registerClass(name, f);
    }

    std::unique_ptr<DataProcessor> getDataProcessor(std::string name) {
        auto result = registry().makeClass(name);
        if(result == nullptr) {
            std::cout << "No DataProcessor matching '" << name << "' found\n";
        }
        return result;
    }

    std::vector<std::string> listDataProcessors() {
        return registry().listClasses();
    }
}

