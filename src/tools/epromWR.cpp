#include <iostream>
#include <iomanip>
#include <string>
#include <stdint.h>

#include <SpecController.h>


int main() {

    // Open device
    SpecController mySpec(0);

    uint32_t offs = 90;
    uint32_t len = 6;
    uint8_t * buffer = new uint8_t[len];
    *(buffer)   = 0xfc;
    *(buffer+1) = 0x0f;
    *(buffer+2) = 0x77;
    *(buffer+3) = 0x07;
    *(buffer+4) = 0x0;
    *(buffer+5) = 0x0;

    mySpec.writeEeprom(buffer, len, offs);

    delete buffer;

    return 0;

}
