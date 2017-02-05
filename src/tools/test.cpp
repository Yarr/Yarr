#include <SpecCom.h>
#include <iostream>
#include <stdint.h>
#include <string.h>

int main(int argc, char **argv) {
    int specNum = 0;
    if (argc == 2)
        specNum = atoi(argv[1]);
    SpecCom mySpec(specNum);
    std::string tmp;
    const size_t size = 256*8;
    unsigned err_count = 0;
    
    uint32_t *data = new uint32_t[size];
    for(unsigned i=0; i<size;i++)
        data[i] = i;

    uint32_t *resp = new uint32_t[size];

    std::cout << "Starting DMA write/read test ..." << std::endl;
    memset(resp, size*4, 0x5A);
    
    mySpec.writeDma(0x0, data, size); 
    std::cout << "... writing " << size * 4 << " byte." << std::endl;
    mySpec.readDma(0x0, resp, size); 
    std::cout << "... read " << size * 4 << " byte." << std::endl;
    
    for (unsigned i=0; i<size; i++) {
        if (data[i] != resp[i]) {
            std::cout << "[" << i << "] " << std::hex << data[i] << " \t " << resp[i] << std::endl << std::dec;
            err_count++;
        }
    }

    if (err_count == 0)
        std::cout << "Success! No errors." << std::endl;
    
    delete[] data;
    delete[] resp;

    return 0;
}
