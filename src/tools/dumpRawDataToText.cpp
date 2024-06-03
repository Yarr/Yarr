#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <bitset>

#include "EventData.h"

void usage(char* argv[])
{
    std::cout << "Usage: " << argv[0] << " inputFile1 inputFile2 ... \n -t <int>: number of triggers in data window" << std::endl;
    exit(1);
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        usage(argv);
        return -1;
    }
    int c;
    unsigned n_trig = 32;

    while ((c = getopt(argc, argv, "ht:")) != -1) {
		switch (c) {
		case 'h':
		    usage(argv);
		    return 0;
		case 't':
		    n_trig = std::stoi(optarg);
		    break;
		default:
		    std::cout << "Invalid command line parameter(s) given!\n";
		    return -1;
	    }
    }

    unsigned globalEventCnt = 0;
    unsigned m_curBlock = 0;

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
        int eventCnt = 0;
        int basetag = 0;
        int exttag = 0; 
        m_curBlock = 0;

        while(inputFile) {
            FrontEndEvent event;
            event.fromFileBinary(inputFile);
            
            // if (event.tag == 0 && m_curBlock > 0) {
            //     std::cout << "Detected new event, sending old unfinished event .. " << eventCnt << std::endl;
            //     eventCnt++;
            //     m_curBlock = 0;
            // }

            basetag = (event.tag & 252) >> 2;
            exttag = (event.tag & 3);    
            prev_tag = event.tag;
	    
            std::cout << 
                std::setw(8) << eventCnt << "|" <<
                std::setw(8) << event.tag << "|" << 
                std::setw(8) << event.l1id << "|" <<
                std::setw(8) << event.bcid << "|" <<
	      std::setw(8) << basetag << "  " << exttag << "   " << std::bitset<12>(event.tag) << "|" <<
                std::setw(8) << event.hits.size() << std::endl;

            for (auto hit : event.hits) {
                std::cout << "       " <<
                    std::setw(4) << hit.col << "|" <<
                    std::setw(4) << hit.row<< "|" <<
                    std::setw(5) << hit.tot << std::endl;
            }

            m_curBlock++;
            if (m_curBlock == n_trig) {
                std::cout << "Sending event .. "<< eventCnt << std::endl;
                eventCnt++;
                m_curBlock = 0;
            }

        }   

    }
    std::cout << "Done, bye!" << std::endl;
    return 0;
}
