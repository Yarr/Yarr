#include <SpecController.h>
#include <iostream>
#include <stdint.h>
#include <string.h>

int main(void) {
    SpecController mySpec(0);
    std::string tmp;
    //std::cin >> tmp;
    const size_t size = 4096;//256*5;
    uint32_t *data = new uint32_t[size];
    uint32_t *resp = new uint32_t[size];
    for(unsigned i=0; i<size; i++)
        data[i] = i;
    //mySpec.writeBlock(0x20000, data, size);
        
    memset(resp, size*4, 0x5A);
    mySpec.writeDma(0x0, data, size); 
    std::cin >> tmp;
    mySpec.readDma(0x0, resp, size); 
//    for(unsigned i=0; i<size; i++)
//        std::cout << "resp[" << i << "] = " << resp[i] << std::endl; 
    
    for (unsigned i=0; i<size; i++)
        if (data[i] != resp[i])
            std::cout << "[" << i << "] " << std::hex << data[i] << " \t " << resp[i] << std::endl << std::dec;
    delete data;
    delete resp;

    return 0;
}
