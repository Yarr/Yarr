#include <SpecController.h>
#include <iostream>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

int main(void) {
    SpecController mySpec(0);
    std::string tmp;
    //std::cin >> tmp;
    const size_t size = 256;//256*5;

    uint32_t enable_mask = 0xFFFFFFFF;
    uint32_t disable_mask = 0x00000000;
    uint32_t data = 0x55555555;
    uint32_t answer = 0;
    
    mySpec.readBlock(0x00008002, &answer, 1);
    std::cout << "Loopback = 0x" << std::hex << answer << std::endl << std::dec;
    std::cout << "Setting Loopback Mode!" << std::endl;
    mySpec.writeBlock(0x00008002, &enable_mask, 1);
    mySpec.readBlock(0x00008002, &answer, 1);
    std::cout << "Loopback = 0x" << std::hex << answer << std::endl << std::dec;

    std::cout << "Writing some data to the Fifo" << std::endl;
    for (unsigned int i=0; i<size; i++)
        mySpec.writeBlock(0x00008004, &data, 1);
    mySpec.readBlock(0x00008003, &answer, 1);
    std::cout << "Rate = " << answer*4.0 << " B/s" << std::endl;
    
    std::cout << "Looking at control Fifo:" << std::endl;
    mySpec.readBlock(0x00008000, &answer, 1);
    std::cout << "Start Adr = 0x" << std::hex << answer << std::endl << std::dec;
    mySpec.readBlock(0x00008001, &answer, 1);
    std::cout << "Count = 0x" << std::hex << answer << std::endl << std::dec;
    mySpec.readBlock(0x00008003, &answer, 1);
    std::cout << "Rate = " << answer*4.0/1024.0 << " kB/s" << std::endl;
    
    mySpec.readBlock(0x00008000, &answer, 1);
    std::cout << "Start Adr = 0x" << std::hex << answer << std::endl << std::dec;
    mySpec.readBlock(0x00008001, &answer, 1);
    std::cout << "Count = 0x" << std::hex << answer << std::endl << std::dec;
    mySpec.readBlock(0x00008003, &answer, 1);
    std::cout << "Rate = " << answer*4.0/1024.0/1024.0 << " MB/s" << std::endl;
    sleep(2);
    mySpec.readBlock(0x00008000, &answer, 1);
    std::cout << "Start Adr = 0x" << std::hex << answer << std::endl << std::dec;
    mySpec.readBlock(0x00008001, &answer, 1);
    std::cout << "Count = 0x" << std::hex << answer << std::endl << std::dec;
    mySpec.readBlock(0x00008003, &answer, 1);
    std::cout << "Rate = " << answer*4.0/1024.0/1024.0 << " MB/s" << std::endl;
    return 0;
}
