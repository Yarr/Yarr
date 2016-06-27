#include <iostream>
#include <fstream>
#include <string>

#include "Histo1d.h"
#include "Histo2d.h"
#include "json.hpp"

using json = nlohmann::json;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "Error: Which file?" << std::endl;
        return 0;
    }
    std::string filename = argv[1];
    std::fstream cfgfile(filename.c_str(), std::ios::in);
    json cfg;
    cfgfile >> cfg;
    
    Histo1d h1("TDacDist", 33, -0.5, 32.5, typeid(void));
    Histo2d h2("TDacMap", 64, 0.5, 64.5, 64, 0.5, 64.5, typeid(void));
    for (unsigned col = 1; col<65; col++) {
        for (unsigned row = 1; row<65; row++) {
            int tdac = cfg["FE65-P2"]["PixelConfig"][col-1]["TDAC"][row-1];
            int sign = cfg["FE65-P2"]["PixelConfig"][col-1]["Sign"][row-1];
            int val = -1;
            if (sign == 0) {
                val = tdac + 16;
            } else {
                val = 15 - tdac;
            }
            h1.fill(val);     
            h2.fill(col, row, val);
        }
    }
    
    h1.plot("config", "");
    h2.plot("config", "");
    return 1;
}
