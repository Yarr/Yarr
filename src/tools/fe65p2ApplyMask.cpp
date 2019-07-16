#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>

#include "json.hpp"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;
int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cout << "Useage:" << std::endl;
        std::cout << "./applyMask mask.dat config.json" << std::endl;
        std::cout << " ~~ Aborting!" << std::endl;
        return -1;
    }

    std::fstream mask(argv[1], std::ios::in);
    std::fstream cfg(argv[2], std::ios::in);

    if (!mask || !cfg) {
        std::cout << "Could not open files!" << std::endl;
        std::cout << " ~~ Aborting!" << std::endl;
        return -1;
    }

    // Parse cfg file
    json c;
    cfg >> c;

    // Skip first 5 lines of mask
    std::string trash;
    for (unsigned i=0; i<5; i++)
        std::getline(mask,trash);

    for (int i=0; i<64; i++) {
        for (int j=0; j<64; j++) {
            int val = -1;
            mask >> val;
            if (val < 1) {
                c["FE65-P2"]["PixelConfig"][j]["PixConf"][i] = 0; // do not enable powerdown
            } else {
                //c["FE65-P2"]["PixelConfig"][j]["PixConf"][i] = 3;
            }
        }
    }

    mask.close();
    cfg.close();

    std::fstream output(("masked_" + std::string(argv[2])).c_str(), std::ios::out);
    if (!output) {
        std::cout << "Couldn't open oputput file!" << std::endl;
        std::cout << " ~~ Aborting!" << std::endl;
        return -1;
    }

    output << std::setw(4) << c << std::endl;

    output.close();



}
