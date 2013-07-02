#include <SpecController.h>

#include <iostream>
#include <sys/time.h>
#include <string.h>
#include <stdint.h>

int main(void) {
    SpecController mySpec(0);
    
    timeval start, end;
    std::cout << std::endl << "==========================================" << std::endl;
    std::cout << "Starting DMA Write Benchmark:" << std::endl;
    for (int cycles=0; cycles<3; cycles++) {
        const size_t size = 1024*100;
        uint32_t data[size];
        // Prepare data pattern
        memset(data, 0x5A, size*4);
        
        // Write to Spec
        int maxLoops = 1;
        gettimeofday(&start, NULL);
        for (int loops=0; loops<maxLoops; loops++)
           mySpec.writeDma(0x0, data, size);
        gettimeofday(&end, NULL);

        // Analyze time
        double total_data = size*4*maxLoops/1024.0/1024.0;
        double time = (end.tv_sec - start.tv_sec) * 1000.0; //msecs
        time += (end.tv_usec - start.tv_usec) / 1000.0; //usecs
        std::cout << "Transferred " << total_data << "MB in " << time << " ms: " << total_data/time*1000.0 << " MB/s" << std::endl;
    }
    std::cout << "===========================================" << std::endl << std::endl;

    std::cout << "===========================================" << std::endl;
    std::cout << "Starting Read Benchmark:" << std::endl;
    for (int cycles=0; cycles<1; cycles++) {
        const size_t size = 1024*100;
        uint32_t data[size];
        
        // Read from Spec
        int maxLoops = 1;
        gettimeofday(&start, NULL);
        for (int loops=0; loops<maxLoops; loops++)
            //mySpec.readDma(0x0, data, size);
        gettimeofday(&end, NULL);

        // Analyze time
        double total_data = size*4*maxLoops/1024.0/1024.0;
        double time = (end.tv_sec - start.tv_sec) * 1000.0; //msecs
        time += (end.tv_usec - start.tv_usec) / 1000.0; //usecs
        std::cout << "Transferred " << total_data << "MB in " << time << " ms: " << total_data/time*1000.0 << " MB/s" << std::endl;
    }
    std::cout << "===========================================" << std::endl << std::endl;

    return 0;
}
