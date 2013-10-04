#include <iostream>
#include <fstream>
#include <string>
#include <stdint.h>

#include <SpecController.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Please specify bitfile to program!" << std::endl;
        return -1;
    }
    
    // Open file
    std::cout << "Opening file: " << argv[1] << std::endl;
    std::fstream file(argv[1], std::ios::binary | std::ios::ate);

    // Get file size
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::cout << "Size: " << size/1024.0 << " kB" << std::endl;

    // Get pointer to file
    char *buffer = new char[size];
    std::cout << "Reading file." << std::endl;
    file.read(buffer, size);

    // Open device
    std::cout << "Opening Spec device." << std::endl;
    SpecController mySpec(0);

    // Start programming
    std::cout << "Starting programming ..." << std::endl;
    unsigned int wrote = mySpec.progFpga(buffer, size);
    if (wrote != size) {
        std::cout << "... error, not all was written!" << std::endl;
        std::cout << " Size:  " << size << std::endl;
        std::cout << " Wrote: " << wrote << std::endl;
        return -1;
    } else {
        std::cout << "... done!" << std::endl;
    }
        
    return 0;
}
