#include <SpecController.h>

#include <iostream>
#include <sys/time.h>
#include <string.h>
#include <stdint.h>

int main(void) {
    SpecController mySpec(0);

    int maxCycles = 10;
    int maxLoops = 50;

    double overall_time = 0;
    double overall_data = 0;

    timeval start, end;
    std::cout << std::endl << "==========================================" << std::endl;
    std::cout << "Starting DMA Write Benchmark:" << std::endl;
    for (int cycles=0; cycles<maxCycles; cycles++) {
        const size_t size = 1024*100; // 400kB
        uint32_t data[size];
        // Prepare data pattern
        memset(data, 0x5A, size*4);
        
        // Write to Spec
        gettimeofday(&start, NULL);
        for (int loops=0; loops<maxLoops; loops++)
           mySpec.writeDma(0x0, data, size);
        gettimeofday(&end, NULL);

        // Analyze time
        double total_data = size*4*maxLoops/1024.0/1024.0;
        overall_data += total_data;
        double time = (end.tv_sec - start.tv_sec) * 1000.0; //msecs
        time += (end.tv_usec - start.tv_usec) / 1000.0; //usecs
        overall_time += time;
        std::cout << "Transferred " << total_data << "MB in " << time << " ms: " << total_data/time*1000.0 << " MB/s" << std::endl;
    }
    std::cout << "===========================================" << std::endl;
    std::cout << "---> Mean Transfer Speed: " << overall_data/overall_time*1000.0 << " MB/s"  << std::endl;
    std::cout << "===========================================" << std::endl << std::endl;

    sleep(1);

    overall_data = 0;
    overall_time = 0;

    std::cout << "===========================================" << std::endl;
    std::cout << "Starting DMA Read Benchmark:" << std::endl;
    for (int cycles=0; cycles<maxCycles; cycles++) {
        const size_t size = 1024*100; // 400kB
        uint32_t data[size];

        // Read from Spec
        gettimeofday(&start, NULL);
        for (int loops=0; loops<maxLoops; loops++)
            mySpec.readDma(0x0, data, size);
        gettimeofday(&end, NULL);

        // Analyze time
        double total_data = size*4*maxLoops/1024.0/1024.0;
        overall_data += total_data;
        double time = (end.tv_sec - start.tv_sec) * 1000.0; //msecs
        time += (end.tv_usec - start.tv_usec) / 1000.0; //usecs
        overall_time += time;
        std::cout << "Transferred " << total_data << "MB in " << time << " ms: " << total_data/time*1000.0 << " MB/s" << std::endl;
    }
    std::cout << "===========================================" << std::endl;
    std::cout << "---> Mean Transfer Speed: " << overall_data/overall_time*1000.0 << " MB/s"  << std::endl;
    std::cout << "===========================================" << std::endl << std::endl;

    return 0;
}
