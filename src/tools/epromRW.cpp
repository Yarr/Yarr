#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdint.h>
#include <string>
#include <time.h>

#include <SpecController.h>


int main() {

    // Open device
    SpecController mySpec(0);

    uint8_t * buffer = new uint8_t[256];
    std::string filepath;

    {
        std::string fnKeyword = ""; //filename keyword put before the time generated filename - use encouraged
        struct tm * timeinfo; 
        time_t rawtime; 
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        
        filepath =   "EEPROMContent/"
                   + fnKeyword + to_string(1900+(timeinfo->tm_year)) + '_' 
                   + to_string(1+(timeinfo->tm_mon)) + '_'
                   + to_string((timeinfo->tm_mday)) + '_'
                   + to_string((timeinfo->tm_hour)) + '_'
                   + to_string((timeinfo->tm_min)) + '_'
                   + to_string((timeinfo->tm_sec)) + ".sbe"; //SpecBoard EEPROM content file (sbe)
    }

    std::ofstream oF(filepath);
    if(!oF) {
        std::perror(filepath);
        exit(-10);
    }

    mySpec.readEeprom(buffer, 256);

    oF << std::hex;
    oF << std::showbase;
    oF << std::setw(9) << "addr" << std::setw(5) << "msk" << std::setw(12) << "data" << std::endl;
    //256/6 = 42; 256%6 = 4
    {
        uint16_t a;     //address
        uint8_t  m;     //mask
        uint32_t d;     //data
        for(unsigned int i = 0; i<42; i++) {
            a  = ((buffer[i*6] | (buffer[i*6+1] << 8)) & 0xffc);
            m  = ((buffer[i*6+1] & 0xf0) >> 4);
            d  = (buffer[i*6+2] | (buffer[i*6+3] << 8) | (buffer[i*6+4] << 16) | (buffer[i*6+5] << 24));
            oF << std::setw(9) << a << std::setw(5) << (int)m << std::setw(12) << d << std::endl;
        }
    }
    oF << std::dec;
    oF << std::noshowbase;

    delete buffer;

    return 0;

}
