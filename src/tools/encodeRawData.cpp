#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include <bitset>

#include "Itkpixv2Encoder.h"
#include "ItkpixEncoder.h"
#include "EventData.h"

int main(int argc, char** argv) {
    std::vector<std::vector<uint16_t> > hitMap;
    hitMap.resize(400);
    for(int i = 0; i < hitMap.size(); i++)
        hitMap[i].push_back(0);

    // std::string path = "/Users/luclepot/Documents/ATLAS/pixels/test/data/011845_bella_exttrigger/0x20c52_data.raw";

    for(int n = 1; n < argc; n++) {
        
        Itkpixv2Encoder *k = new Itkpixv2Encoder();
        k->setEventsPerStream(1);

        std::string path = std::string(argv[n]);
        std::cout << "Opening file: " << path << std::endl;
        std::fstream inputFile(path, std::fstream::in | std::fstream::binary);
        if(!inputFile.good()) {
            std::cout << "FAILED!" << std::endl;
            continue;
        }
        
        FrontEndEvent evo;
        int i = 0;
        int n_hits = 0;
        int n_events = 0;
        // std::vector<int> word_counts;
        int n_words = 0;
        while(inputFile) {
            evo.fromFileBinary(inputFile);
            i++;
            // if(i % 10 == 0) {
            // }
            for(int col = 0; col < hitMap.size(); col++) {
                for(int row = 0; row < hitMap[col].size(); row++) {
                    hitMap[col][row] = 0;
                }
            }
            for(int hit_n = 0; hit_n < evo.hits.size(); hit_n++) {
                // std::cout << "hit_n: " << hit_n << std::endl;
                
                hitMap[evo.hits[hit_n].row][evo.hits[hit_n].col] = evo.hits[hit_n].tot;
                n_hits++;
            }
            // if(i == 395) {
            //     continue;
            //     // std::cout << evo.hits[hit_n].row << ", " << evo.hits[hit_n].col << ", " << evo.hits[hit_n].tot << std::endl;
            // }
            k->addToStream(hitMap, true, false);
            n_words += k->wordSize();
            k->flushWords();
            std::cout << "   Event " << i << ": hit size " << evo.hits.size() << ", word_count " << k->wordSize() << std::endl;

            evo.hits.clear();
            evo.nHits = 0;
            n_events++;
        }

        std::cout << "Got to encoding part.." << std::endl;
        
        // int n_words = 0;
        // for(int i = 0; i < word_counts.size(); i++) {
        //     n_words += word_counts[i];
        // }
        
        
        std::cout << "test main\n";
        std::cout << "Encoded word size: " << n_words << ", with " << 32*n_words << "bits\n";
        std::cout << "Hits: " << n_hits << ", Events: " << n_events << "\n";
        std::cout << "Bits/Event: " << ((float)n_words*32)/((float)n_events) << std::endl;
    }
    return 0;
}