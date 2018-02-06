#include <functional>
#include <map>
#include <string>

template <typename T>
class ClassRegistry {

    typedef std::function<std::unique_ptr<T>()> FunctionType;
    typedef std::map<std::string, FunctionType> MapType;

    MapType registry;

 public:

    bool registerClass(std::string name,
                       FunctionType func) {
        registry[name] = func;
        return true;
    }

    std::unique_ptr<T> makeClass(std::string name) {
        try {
            return registry.at(name)();
        } catch(std::out_of_range &e) {
            return nullptr;
        }
    }

    std::vector<std::string> listClasses() {
        std::vector<std::string> known;
        for (auto &i: registry) {
            known.push_back(i.first);
        }
        return known;
    }
};
