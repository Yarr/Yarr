#include <SpecCom.h>

#include <iostream>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <cmath>

uint32_t rand32() {
    uint32_t number = 0;
    number += rand() % 256;
    number += (rand() % 256) << 8;
    number += (rand() % 256) << 16;
    number += (rand() % 256) << 24;
    return number;
}


int main (void) {
    SpecCom mySpec(0);
    
    int maxLoops = 50000;
    int overall_errors = 0;
    uint32_t off = 0;

    const size_t size = 256*100; // 1kB
    
    srand(time(NULL));

    std::cout << "==================================" << std::endl;
    std::cout << "Starting error check:" << std::endl;
    std::cout << "Sample size: " << size/256.0 << " kB" << std::endl;
    std::cout << "Iterations: " << maxLoops << std::endl;
    std::cout << "==================================" << std::endl;
    
    int cur = 0;
    int cur_old = -1;
    for (int loop = 0; loop<maxLoops; loop++) {
        cur = fabs(loop/(double)maxLoops*100);
        if (cur != cur_old) {
            std::cout << "\r|";
            for(int i=0; i<20; i++) {
                if (i*5 <= cur) {
                    std::cout << "#";
                } else {
                    std::cout << " ";
                }
            }
            std::cout << "| " << (int) cur << "%";
            std::cout.flush();
        }
        cur_old = cur;
        //std::cout << std::endl << "==================================" << std::endl;
        
        //std::cout << "Creating Sample of size " << size*4/1024 << "kB ... ";
        uint32_t *sample = new uint32_t[size];
        for(unsigned int i = 0; i<size; i++)
            //sample[i] = rand32();
            sample[i] = (i+off);

        //std::cout << "Writing Sample ... ";
        if (mySpec.writeDma(off, sample, size))
            return 0; 
        //std::cout << "Reading Sample!" << std::endl;
        uint32_t *readBack = new uint32_t[size];
        if (mySpec.readDma(off, readBack, size))
            return 0;

        int counter = 0;
        //std::cout << "Sample\tReadback #" << loop << " " <<std::endl;
        for(unsigned int i = 0; i<size; i++) {
            if(sample[i] != readBack[i]) {
                counter ++;
                std::cout << loop << " [" << i << "] " << std::hex << sample[i] << "\t" << readBack[i] << std::dec << std::endl;
            }
        }
        //std::cout << "Found #" << counter << " errors!" << std::endl;
        //std::cout << "==================================" << std::endl;
        overall_errors += counter;
        //if (counter != 0) return 0;
        off += size;
        off = off%0x10000000;

        delete[] sample;
        delete[] readBack;
        if (counter > 0)
            return 0;
        //sleep(1); // for chipscope

    }
    
    std::cout << std::endl;
    std::cout << std::endl << "==================================" << std::endl;
    std::cout << "Total Data transfered " << size*4*maxLoops/1024.0/1024.0/1024.0 << " GB!" << std::endl;
    std::cout << "Total Number of Errors #" << overall_errors << " errors!" << std::endl;
    std::cout << (double)overall_errors/(double)(size*maxLoops) << "\% are errors!" << std::endl;
    std::cout << "Bit error rate (CL 95%): " << (double)(-1*log(0.05))/(double)(size*maxLoops*32) << std::endl;
    std::cout << "==================================" << std::endl;

    return 0;
}
