#include <iostream>
#include <iomanip>
#include <string>
#include <stdint.h>

#include <SpecController.h>


int main() {

    // Open device
    std::cout << "Opening Spec device." << std::endl; //debug
    SpecController mySpec(0);
    std::cout << "Opened SpecController.\n"; //debug

    //uint32_t len = 256;
    //uint8_t * buffer = new uint8_t[len];
    uint8_t * buffer = new uint8_t[256];

    //debug
    //std::cout << "Reading Gennum registers before EEPROM read for comparison.\n";
    //mySpec.readGennumRegs();

    std::cout << "Created new array.\nCalling read function.\n"; //debug

    mySpec.readEeprom(buffer, 256);

    std::cout << "Read function returned.\nReading array:\n"; //debug

    std::cout << std::hex;
    std::cout << std::showbase;
    std::cout << std::setw(9) << "addr" << std::setw(5) << "msk" << std::setw(12) << "data" << std::endl;
    //256/6 = 85; 256%6 = 2
    {
        uint16_t a;     //address
        uint8_t  m;     //mask
        uint32_t d;     //data
        for(unsigned int i = 0; i<85; i++) {
            a  = ((buffer[i*6] | (buffer[i*6+1] << 8)) & 0xffc);
            m  = ((buffer[i*6+1] & 0xf0) >> 4);
            d  = (buffer[i*6+2] | (buffer[i*6+3] << 8) | (buffer[i*6+4] << 16) | (buffer[i*6+5] << 24));
            std::cout << std::setw(9) << a << std::setw(5) << (int)m << std::setw(12) << d << std::endl;
        }
    }
    std::cout << std::dec;
    std::cout << std::noshowbase;

    std::cout << "Deleting array.\n"; //debug

    delete buffer;

    //debug
    //std::cout << "Reading Gennum registers after EEPROM read for comparison.\n";
    //mySpec.readGennumRegs();

    std::cout << "Returning control.\n"; //debug

    return 0;

}
