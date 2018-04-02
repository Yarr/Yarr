#include <iostream>
#include <fstream>

#include "Rd53a.h"
#include "json.hpp"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

int main(int argc, char *argv[]) {
    
    // Init
    Rd53a fe;
    
    // Load config
    if (argc > 2) {
        json cfg;
        std::ifstream cfgFile(argv[2]);
        cfg = json::parse(cfgFile);
        fe.fromFileJson(cfg);
        cfgFile.close();
    }

    //fe.setEn(1, 0, 1);


    // Write config
    if (argc > 1) {
        json cfg;
        fe.toFileJson(cfg);
        std::cout << cfg.dump(4) << std::endl;
        std::ofstream cfgFile(argv[1]);
        cfgFile << cfg.dump(4);
    }

    return 0;
}
