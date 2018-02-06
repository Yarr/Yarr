#include <functional>
#include <map>
#include <string>

template <typename T, typename... A>
class ClassRegistry {

    typedef std::function<std::unique_ptr<T>(A... args)> FunctionType;
    typedef std::map<std::string, FunctionType> MapType;

    MapType registry;

 public:

    bool registerClass(std::string name,
                       FunctionType func) {
        registry[name] = func;
        return true;
    }

    std::unique_ptr<T> makeClass(std::string name, A... args) {
        try {
            return registry.at(name)(args...);
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
