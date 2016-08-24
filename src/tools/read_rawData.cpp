#include <iostream>
#include <fstream>

#include "Fei4EventData.h"
#include "Histo1d.h"
#include "Histo2d.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Provide input file!" << std::endl;
        return -1;
    }

    for (int i=1; i<argc; i++) {
        std::cout << "Opening file: " << argv[i] << std::endl;
        std::fstream file(argv[i], std::fstream::in | std::fstream::binary);

        file.seekg(0, std::ios_base::end);
        int size = file.tellg();
        file.seekg(0, std::ios_base::beg);
        std::cout << "Size: " << size/1024.0/1024.0 << " MB" << std::endl;        

        int count = 0;
        while (file) {
            Fei4Event event;
            event.fromFileBinary(file);
            if (!file)
                break;
            if(event.nHits > 0) {
                std::cout << "~~ Event #" << count << " ~~" << std::endl;
                std::cout << event.l1id << " " << event.bcid << " " << event.nHits << std::endl;
                for (unsigned i=0; i<event.nHits; i++) {
                    std::cout << event.hits[i].col << " " << event.hits[i].row << " " << event.hits[i].tot << std::endl;
                }
            }
            count ++;
        }

        file.close();
    }

    return 0;
}
