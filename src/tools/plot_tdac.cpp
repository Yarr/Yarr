#include <iostream>
#include <fstream>
#include <string>

#include "Histo1d.h"
#include "Histo2d.h"
#include "json.hpp"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "Error: Which file?" << std::endl;
        return 0;
    }
    std::string filename = argv[1];
    std::fstream cfgfile(filename.c_str(), std::ios::in);
    json cfg;
    cfgfile >> cfg;
    
    Histo1d h1("TDacDistLin", 16, -0.5, 15.5, typeid(void));
    Histo1d h2("TDacDistDiff", 31, -15.5, 15.5, typeid(void));
    Histo2d h3("TDacMap", 400, 0.5, 400.5, 192, 0.5, 192.5, typeid(void));
    for (unsigned col = 128; col<400; col++) {
        for (unsigned row = 0; row<192; row++) {
            int tdac = cfg["RD53A"]["PixelConfig"][col]["TDAC"][row];
	    if (col<264){
            h1.fill(tdac);
	    }else{
            h2.fill(tdac);
	    }
            h3.fill(col, row, tdac);
        }
    }
    h1.plot("config", "");
    h2.plot("config", "");
    h3.plot("config", "");
    return 1;
}
