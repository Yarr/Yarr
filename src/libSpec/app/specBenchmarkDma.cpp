#include <SpecCom.h>

#include <iostream>
#include <fstream>
#include <sys/time.h>
#include <string.h>
#include <stdint.h>
#include <chrono>
#include <ctime>

int main(int argc, char *argv[]) {
    unsigned spec_num = 0;
    if (argc == 2)
        spec_num = atoi(argv[1]);
    SpecCom mySpec (spec_num);

    std::fstream file_write("benchmarkDma_write.out", std::ios::out);
    std::fstream file_read("benchmarkDma_read.out", std::ios::out);

    int maxCycles = 400; // 400 
    int maxLoops = 1;  // 100

    double overall_time = 0;
    double overall_data = 0;

    std::chrono::time_point<std::chrono::system_clock> start, end;

    mySpec.resetFIFO();

    std::cout << std::endl << "==========================================" << std::endl;
    std::cout << "Starting DMA Write Benchmark:" << std::endl;
    for (int cycles=0; cycles<maxCycles; cycles++) {
        const size_t size = 256*(cycles+1);
        uint32_t *data = new uint32_t[size];
        // Prepare data pattern
        memset(data, 0x5A, size*4);
        
        // Write to Spec
	start = std::chrono::system_clock::now();
        for (int loops=0; loops<maxLoops; loops++)
           if (mySpec.writeDma(0x0, data, size)) return 1;
	end = std::chrono::system_clock::now();

	std::chrono::duration<double> elapsed_seconds = end - start;
	double time = elapsed_seconds.count()*1000;

        // Analyze time
        double total_data = size*4*maxLoops/1024.0/1024.0;
        overall_data += total_data;
        overall_time += time;
        std::cout << "Transferred " << total_data << "MB in " << time << " ms: " << total_data/time*1000.0 << " MB/s" << std::endl;
	file_write << size << "\t" << total_data/time*1000.0 << std::endl;
	delete data;
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
        uint32_t *data = new uint32_t[size];

        // Read from Spec
	start = std::chrono::system_clock::now();
        for (int loops=0; loops<maxLoops; loops++)
           if (mySpec.readDma(0x0, data, size)) return 1;
	end = std::chrono::system_clock::now();

	std::chrono::duration<double> elapsed_seconds = end - start;
	double time = elapsed_seconds.count()*1000;

        // Analyze time
        double total_data = size*4*maxLoops/1024.0/1024.0;
        overall_data += total_data;
        overall_time += time;
        std::cout << "Transferred " << total_data << "MB in " << time << " ms: " << total_data/time*1000.0 << " MB/s" << std::endl;
	file_read << size << "\t" << total_data/time*1000.0 << std::endl;
	delete data;
    }
    std::cout << "===========================================" << std::endl;
    std::cout << "---> Mean Transfer Speed: " << overall_data/overall_time*1000.0 << " MB/s"  << std::endl;
    std::cout << "===========================================" << std::endl << std::endl;
    
    file_write.close();
    file_read.close();
    return 0;
}
