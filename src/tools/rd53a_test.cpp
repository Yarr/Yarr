#include <iostream>

#include "Rd53a.h"

int main(void) {
    //Rd53a myRd53a(NULL);
    
    std::cout << "Test encoding ..." << std::endl;
    for (uint32_t i=0; i<32; i++)
        std::cout << "[" << i << "] = " << std::hex << "0x" << Rd53a::encode5to8(i) << std::dec << std::endl;

    return 0;
}
