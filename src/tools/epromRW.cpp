#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdint.h>
#include <string>
#include <time.h>

#include <SpecCom.h>

int main(int argc, char * argv[]) {

    std::string fnKeyword = ""; //filename keyword put before the time generated filename - use encouraged
    if(argc == 1) {
        ;
    } else if (argc == 2) {
        fnKeyword = argv[1];
    } else {
        std::cout << "Too many arguments. Aborting... \n";
    }

    // Open device
    SpecCom mySpec(0);

    uint8_t * buffer = new uint8_t[ARRAYLENGTH];

    mySpec.readEeprom(buffer, ARRAYLENGTH);

    mySpec.createSbeFile(fnKeyword, buffer, ARRAYLENGTH);

    delete[] buffer;

    return 0;

}
