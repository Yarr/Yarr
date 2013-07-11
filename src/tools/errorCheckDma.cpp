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

    int maxLoops = 100;
    int overall_errors = 0;
    uint32_t off = 0;

    const size_t size = 1024/4*20; // 1kB
    
    srand(time(NULL));

    for (int loop = 0; loop<maxLoops; loop++) {
        std::cout << std::endl << "==================================" << std::endl;
        
        std::cout << "Creating Sample of size " << size*4/1024 << "kB ... ";
        uint32_t sample[size];
        for(unsigned int i = 0; i<size; i++)
            //sample[i] = rand32();
            sample[i] = i;

        std::cout << "Writing Sample ... ";
        mySpec.writeDma(off, sample, size);
        
        //std::string tmp;
        //std::cin >> tmp;

        std::cout << "Reading Sample!" << std::endl;
        uint32_t readBack[size];
        mySpec.readDma(off, readBack, size);

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
        overall_errors += counter;
        if (counter != 0) return 0;
        off += size;
        sleep(2); // is this needed?
    }
        std::cout << std::endl << "==================================" << std::endl;
        std::cout << "Total Number of Errors #" << overall_errors << " errors!" << std::endl;
        std::cout << (double)overall_errors/(double)(size*maxLoops) << "\% are errors!" << std::endl;
        std::cout << "==================================" << std::endl;

    return 0;
}
