#include <SpecCom.h>

#include <iostream>
#include <fstream>
#include <sys/time.h>
#include <string.h>
#include <stdint.h>

int main(int argc, char *argv[]) {
    unsigned spec_num = 0;
    if (argc == 2)
        spec_num = atoi(argv[1]);
    SpecCom mySpec (spec_num);

    std::fstream file_write("benchmarkDma_write.out", std::ios::out);
    std::fstream file_read("benchmarkDma_read.out", std::ios::out);

    int maxCycles = 400;
    int maxLoops = 100;

    double overall_time = 0;
    double overall_data = 0;

    timeval start, end;

    std::cout << std::endl << "==========================================" << std::endl;
    std::cout << "Starting DMA Write Benchmark:" << std::endl;
    for (int cycles=0; cycles<maxCycles; cycles++) {
        const size_t size = 256*(cycles+1);
        uint32_t data[size];
        // Prepare data pattern
        memset(data, 0x5A, size*4);
        
        // Write to Spec
        gettimeofday(&start, NULL);
        for (int loops=0; loops<maxLoops; loops++)
           if (mySpec.writeDma(0x0, data, size)) return 1;
        gettimeofday(&end, NULL);

        // Analyze time
        double total_data = size*4*maxLoops/1024.0/1024.0;
        overall_data += total_data;
        double time = (end.tv_sec - start.tv_sec) * 1000.0; //msecs
        time += (end.tv_usec - start.tv_usec) / 1000.0; //usecs
        overall_time += time;
        std::cout << "Transferred " << total_data << "MB in " << time << " ms: " << total_data/time*1000.0 << " MB/s" << std::endl;
        file_write << size << "\t" << total_data << "\t" << time << "\t" << total_data/time*1000.0 << std::endl;
    }
    std::cout << "===========================================" << std::endl;
    std::cout << "---> Mean Transfer Speed: " << overall_data/overall_time*1000.0 << " MB/s"  << std::endl;
    std::cout << "===========================================" << std::endl << std::endl;

    overall_data = 0;
    overall_time = 0;


    std::cout << "===========================================" << std::endl;
    std::cout << "Starting DMA Read Benchmark:" << std::endl;
    for (int cycles=0; cycles<maxCycles; cycles++) {
        const size_t size = 256*(cycles+1);
        uint32_t data[size];

        // Read from Spec
        gettimeofday(&start, NULL);
        for (int loops=0; loops<maxLoops; loops++)
            if(mySpec.readDma(0x0, data, size)) return 1;
        gettimeofday(&end, NULL);
        
        // Analyze time
        double total_data = size*4*maxLoops/1024.0/1024.0;
        overall_data += total_data;
        double time = (end.tv_sec - start.tv_sec) * 1000.0; //msecs
        time += (end.tv_usec - start.tv_usec) / 1000.0; //usecs
        overall_time += time;
        std::cout << "Transferred " << total_data << "MB in " << time << " ms: " << total_data/time*1000.0 << " MB/s" << std::endl;
        file_read << size << "\t" << total_data << "\t" << time << "\t" << total_data/time*1000.0 << std::endl;
    }
    std::cout << "===========================================" << std::endl;
    std::cout << "---> Mean Transfer Speed: " << overall_data/overall_time*1000.0 << " MB/s"  << std::endl;
    std::cout << "===========================================" << std::endl << std::endl;
    
    file_write.close();
    file_read.close();
    return 0;
}
