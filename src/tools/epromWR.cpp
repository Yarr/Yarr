#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <stdint.h>

#include <SpecCom.h>

int main(int argc, char * argv[]) {

    // Open device
    SpecCom mySpec(0);

    if((argc == 1) || (argc > 2)) {
        std::cout << "Requires one argument (path to .sbe file). Aborting... \n";
        exit(-1);
    }
    std::string pathname;
    pathname = argv[1];
    uint8_t * buffer = new uint8_t[ARRAYLENGTH];
    mySpec.getSbeFile(pathname, buffer, ARRAYLENGTH);

    mySpec.writeEeprom(buffer, ARRAYLENGTH, 0);

    delete[] buffer;

    return 0;

}
