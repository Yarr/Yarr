#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>

#include "EventData.h"

void usage(char* argv[])
{
    std::cout << "Usage: " << argv[0] << " inputFile1 inputFile2 ..." << std::endl;
    exit(1);
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        usage(argv);
        return -1;
    }

    unsigned globalEventCnt = 0;
    for(int n=1; n<argc; n++) {
        std::string inputFilePath = std::string(argv[n]);
        std::cout << "Opening file: " << inputFilePath << std::endl;
        
        std::fstream inputFile(inputFilePath, std::fstream::in | std::fstream::binary);
        if (!inputFile.good()) {
            std::cerr << " ... could not open file, skipping ..." << std::endl;
            continue;
        }

        int prev_tag = 31;
        unsigned localEventCnt = 0;
        while(inputFile) {
            FrontEndEvent event;
            event.fromFileBinary(inputFile);
            
            if ((event.tag+32-prev_tag)%32 != 1) {
                std::cout << " ~~~~ Unfinished event ~~~~ " << std::endl;
                if (event.tag != 0) {
                    std::cout << " ~~~~ Middle skip ~~~~ " << std::endl;
                }
            }
            prev_tag = event.tag;

            std::cout << 
                std::setw(8) << globalEventCnt << "|" <<
                std::setw(8) << localEventCnt << "|" <<
                std::setw(8) << event.l1id << "|" <<
                std::setw(8) << event.bcid << "|" <<
                std::setw(8) << event.tag << "|" <<
                std::setw(8) << event.hits.size() << std::endl;

            for (auto hit : event.hits) {
                std::cout << "       " <<
                    std::setw(4) << hit.col << "|" <<
                    std::setw(4) << hit.row<< "|" <<
                    std::setw(5) << hit.tot << std::endl;
            }
            

            globalEventCnt++;
            localEventCnt++;

        }

    }

    std::cout << "Done, bye!" << std::endl;
    return 0;
}
