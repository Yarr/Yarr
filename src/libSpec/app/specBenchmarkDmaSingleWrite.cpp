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
	int cycleNb = 1;
	if (argc == 2)
		spec_num = atoi(argv[1]);
	SpecCom mySpec (spec_num);
	if (argc == 3)
		cycleNb = atoi(argv[2]);

	std::ofstream file_write;
	
	if (cycleNb == 1) {
		mySpec.resetFIFO();
		file_write.open("benchmarkDma_write.out", std::ios::out);
	}
	else 
		file_write.open("benchmarkDma_write.out", std::ios::out|std::ios::app);

	int maxLoops = 1;  // 100

	double overall_time = 0;
	double overall_data = 0;

	std::chrono::time_point<std::chrono::system_clock> start, end;

        const size_t size = 256*(cycleNb);
        uint32_t *data = new uint32_t[size];

        // write from Spec
	start = std::chrono::system_clock::now();
        for (int loops=0; loops<maxLoops; loops++)
            if(mySpec.writeDma(0x0, data, size)) return 1;
	end = std::chrono::system_clock::now();
        
        // Analyze time
        double total_data = size*4*maxLoops/1024.0/1024.0;
        overall_data += total_data;

	std::chrono::duration<double> elapsed_seconds = end - start;
	double time = elapsed_seconds.count()*1000;

        overall_time += time;
        std::cout << "Transferred " << total_data << "MB in " << time << " ms: " << total_data/time*1000.0 << " MB/s" << std::endl;

	//file_write << size << "\t" << total_data << "\t" << time << "\t" << total_data/time*1000.0 << std::endl;
	file_write << size << "\t" << total_data/time*1000.0 << std::endl;

	delete data;

	file_write.close();

	return 0;
}
