#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <stdint.h>

#include <SpecController.h>


int main(int argc, char * argv[]) {

    // Open device
    SpecController mySpec(0);

    uint32_t len = 252;
    uint8_t * buffer = new uint8_t[len];
    if((argc == 1) || (argc > 2)) {
        std::cout << "Requires one argument (path to .sbe file). Aborting... \n";
        exit(-30);
    }
    std::string pathname;
    pathname = argv[1];
    std::ifstream iF(pathname);

    {
        std::string tmpStr;
        std::getline(iF, tmpStr); //Headline - add sanity check if desired
        if(tmpStr != "     addr  msk        data") {
            std::cout << "Invalid headline. \n "
                      << "Note that it is encouraged to automatically create an sbe file \n"
                      << "using the EEPROM read functionality, copy the file, do desired \n"
                      << "changes by hand and use this file to write to the EEPROM. \n";
            return -31;
        }
    }

    {
        uint32_t tmpInt;
        unsigned int i = 0;
        iF >> std::hex;
        iF >> std::showbase;
        while(iF >> tmpInt) {
            switch(i%3) {
            case 0:
                if(tmpInt>0xFFF) {
                    std::cout << "Invalid address size in line " << i/3 + 1 << ". Aborting... \n";
                    return -32
                }
                *(buffer+i) = (tmpInt & 0xFF);
                i++;
                *(buffer+i) = ((tmpInt >> 8) & 0xF);
                break;
            case 1:
                if(tmpInt>0xF) {
                    std::cout << "Invalid mask size in line " << i/3 + 1 << ". Aborting... \n";
                    return -33;
                }
                *(buffer+i) |= ((tmpInt & 0xF) << 4);
                i++;
                break;
            case 2:
                if(tmpInt>0xFFFFFFFF) {
                    std::cout << "Invalid data size in line " << i/3 + 1 << ". Aborting... \n";
                    return -34;
                }
                *(buffer+i)   = (tmpInt & 0xFF);
                *(buffer+i+1) = ((tmpInt >>  8) & 0xFF);
                *(buffer+i+2) = ((tmpInt >> 16) & 0xFF);
                *(buffer+i+3) = ((tmpInt >> 24) & 0xFF);
                i+=4; //equivalent i->i+3+1, case structure invariant under i->i+3
                break;
            }
        }
        iF >> std::dec;
        iF >> std::noshowbase;
        if(i!=252) {
            std::cout << "Input file incomplete or too big. Aborting... \n";
            return -20;
        }
    }

    mySpec.writeEeprom(buffer, len, 0);

    iF.close();
    delete buffer;

    return 0;

}
