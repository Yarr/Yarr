#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <set>
#include <iomanip>
#include <bitset>
#include <unistd.h> 

#include "EventData.h"

void usage(char* argv[])
{   
    std::cout << "Usage: " << argv[0] << " inputFile1 inputFile2 ... \n"
        " -t <int>: number of triggers in data window\n"
        " -s <int>: start position (default 0)\n"
        " -e <int>: end position (default -1)\n"
        " -j : use trigger tagging logic (default false)\n"
        " -n : no printing hit info (default false)\n"
        " -r <list>: comma-separated list of tags to ignore (e.g., -r 10,12,15)" << std::endl;
    exit(1);
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        usage(argv);
        return -1;
    }
    
    int c;
    unsigned n_trig = 0;
    unsigned long n_start = 0, n_end = -1;
    bool use_trigtags = false;
    bool no_hits = false;
    std::set<int> ignore_tags;

    while ((c = getopt(argc, argv, "hjt:s:e:nr:")) != -1) {
        switch (c) {
        case 'h': usage(argv); return 0;
        case 't': n_trig = std::stoi(optarg); break;
        case 's': n_start = std::stoi(optarg); break;
        case 'e': n_end = std::stoi(optarg); break;
        case 'j': use_trigtags = true; break;
        case 'n': no_hits = true; break;
        case 'r': {
            std::stringstream ss(optarg);
            std::string segment;
            while(std::getline(ss, segment, ',')) {
                ignore_tags.insert(std::stoi(segment));
            }
            break;
        }
        default: usage(argv); return -1;
        }
    }

<<<<<<< HEAD
    unsigned m_curBlock = 0;
=======
    // Print Header
    std::cout << "EventCnt |    Tag |   L1ID |   BCID | Base/ExtTag | Tag(bin) | HitCount | Index" << std::endl;
    std::cout << "---------|---------|---------|---------|-------------|----------|----------|--------" << std::endl;
    if (!no_hits) {
        std::cout << "  Col    |   Row   |   ToT" << std::endl;
        std::cout << "--------------------------" << std::endl;
    }
>>>>>>> 9282572a (add headders and new command line arguments to dataDumper)

    for(int n=optind; n<argc; n++) {
        std::fstream inputFile(argv[n], std::fstream::in | std::fstream::binary);
        if (!inputFile.good()) continue;

<<<<<<< HEAD

        int eventCnt = 0;

        m_curBlock = 0;
        
        FrontEndEvent evo;
        evo.fromFileBinary(inputFile);

        int basetag = 0;
        int exttag = 0;
        int prev_basetag = 0;
        unsigned i = 0;
=======
        unsigned eventCnt = 0;
        unsigned m_curBlock = 0;
        int i = 0;
        int basetag = 0, exttag = 0, prev_basetag = 0;
>>>>>>> 9282572a (add headders and new command line arguments to dataDumper)

        while(inputFile) {
            FrontEndEvent event;
            event.fromFileBinary(inputFile);
            
            if(i < n_start) { i++; continue; }
            else if (i > n_end && n_end != (unsigned long)-1) break;

            // Reject Filter: Skip processing if tag is in the list
            if (ignore_tags.find(event.tag) != ignore_tags.end()) {
                i++;
                continue;
            }

            basetag = (event.tag & 252) >> 2;
            exttag = (event.tag & 3);

            if(((basetag - prev_basetag) > 1) & (i > n_start) & (use_trigtags)) {
                std::cout << "ERROR: basetag jump: ";
            }

            std::cout << std::setw(8) << eventCnt << "|" 
                      << std::setw(8) << event.tag << "|"
                      << std::setw(8) << event.l1id << "|"
                      << std::setw(8) << event.bcid << "|"
                      << std::setw(8) << basetag << "  " << exttag << "   " 
                      << std::bitset<12>(event.tag) << "|"
                      << std::setw(8) << event.hits.size() << "|" 
                      << std::setw(8) << i << std::endl;

            // Full hit printing logic restored
            if (!no_hits) {
                for (auto hit : event.hits) {
                    std::cout << "     " <<
                        std::setw(4) << hit.col << "|" <<
                        std::setw(4) << hit.row << "|" <<
                        std::setw(5) << hit.tot << std::endl;
                }
            }

            m_curBlock++;
            if (m_curBlock == n_trig) {
                std::cout << "Sending event .. "<< eventCnt << std::endl;
                eventCnt++;
                m_curBlock = 0;
            }

            prev_basetag = basetag;
            i++;
        }
    }
    std::cout << "Done, bye!" << std::endl;
    return 0;
}
