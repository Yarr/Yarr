#include <SpecController.h>

#include <iostream>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

uint32_t rand32() {
    uint32_t number = 0;
    number += rand() % 256;
    number += (rand() % 256) << 8;
    number += (rand() % 256) << 16;
    number += (rand() % 256) << 24;
    return number;
}


int main (void) {
    SpecController mySpec(0);

    int maxLoops = 10;
    
    const size_t size = 1024/4*10; // 1kB
    
    srand(time(NULL));

    for (int loop = 0; loop<maxLoops; loop++) {
        std::cout << std::endl << "==================================" << std::endl;
        
        std::cout << "Creating Sample of size " << size*4/1024 << "kB ... ";
        uint32_t sample[size];
        for(unsigned int i = 0; i<size; i++)
            //sample[i] = rand32();
            sample[i] = i;

        std::cout << "Writing Sample ... ";
        mySpec.writeBlock(0x10000, sample, size);
        
        //std::string tmp;
        //std::cin >> tmp;

        std::cout << "Reading Sample!" << std::endl;
        uint32_t readBack[size];
        mySpec.readDma(0x0, readBack, size);

        int counter = 0;
        std::cout << "Sample\tReadback" << std::endl;
        for(unsigned int i = 0; i<size; i++) {
            if(sample[i] != readBack[i]) {
                counter ++;
                std::cout << "[" << i << "] " << std::hex << sample[i] << "\t" << readBack[i] << std::dec << std::endl;
            }
        }
        std::cout << "Found #" << counter << " errors!" << std::endl;
        std::cout << "==================================" << std::endl;
    }

    return 0;
}
