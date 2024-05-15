#include "catch.hpp"
#include "Rd53bEncodingTool.h"
#include "AllProcessors.h"

#include "EventData.h"
#include "Rd53bCfg.h"
#include <memory>
#include <iostream>
#include <bitset>
#include <algorithm>
#include "../tests/rd53b_test_stream.h"

int main(){
    std::cout << "Encoding portability tester\n";
    std::unique_ptr<Rd53bEncodingTool> encoder(new Rd53bEncodingTool());
    encoder->setSeed(1654892);
    encoder->setEventsPerStream(1);
    //encoder->generate(28, 1e-3, 2);
    encoder->basicTestEvent();
    //std::vector<Rd53bEncodingTool::hitCoordinate> truth = encoder->getTruthHits();
    FrontEndData truthData = encoder->getTruthData();
    std::vector<uint32_t> words_v = encoder->getWords();
    std::cout << words_v.size() << "\n";

    for (uint32_t w : words_v){
        std::bitset<32> bw(w);
        std::cout << bw.to_string() << "\n";
    }
    return 0;
    //mockup
    //std::vector<uint32_t> words_v;
    //words_v.reserve(nWords);
    //for (uint i = 0; i < nWords; i++) words_v.push_back(words[i]);

    //std::cout << "Total " << truth.size() << " hits encoded into " << words_v.size() << " words.\n";

    //invoke the processor
    std::shared_ptr<FeDataProcessor> proc = StdDict::getDataProcessor("RD53B");

    //these input (rd, raw data) and output (em) clipboards are to share data between different processes
    ClipBoard<RawDataContainer> rd_cp;
    ClipBoard<EventDataBase> em_cp;

    //chip config
    Rd53bCfg cfg;  

    //give the processor the configuration, input and output containers
    proc->connect(&cfg, &rd_cp, &em_cp );

    //turn on the processor
    proc->init();
    proc->run();

    //create raw data container and put the generated data into it
    RawDataPtr rd = std::make_shared<RawData>(0, words_v.size());
    //RawDataPtr rd = std::make_shared<RawData>(0, nWords);
    uint32_t *buffer = rd->getBuf();
    buffer[words_v.size()-1] = 0;
    //buffer[nWords-1] = 0;


    std::copy(words_v.data(), words_v.data()+words_v.size(), buffer);
    //std::copy(words, words+nWords, buffer);

    std::cout << "buffer: " << buffer[0] << " " << buffer[1] << " " << buffer[2] << "\n";
    //std::cout << "words:" << words[0] << " " << words[1] << " " << words[2] << "\n";
    std::cout << "words:" << words_v[0] << " " << words_v[1] << " " << words_v[2] << "\n";

    //put the raw data into a higher order container and throw it into the input clipboard
    std::unique_ptr<RawDataContainer> rdc(new RawDataContainer(LoopStatus()));
    rdc->add(std::move(rd));
    rd_cp.pushData(std::move(rdc));

    rd_cp.finish();
    
    //hold the thread
    proc->join();

    //get the output data
    auto data = em_cp.popData();

    FrontEndData &rawData = *(FrontEndData*)data.get();

    std::cout << "We've decoded " << rawData.events.size() << " events.\n";
    std::cout << "out of " << truthData.events.size() << " truth events.\n";

    int ih = 0;
    int ie = 0;
    //for (auto &event : rawData.events){
    //    std::cout << "Event " << ie << "\n";
    //    for(auto &hit : event.hits){
    //        std::cout << "DECODED: col: " << hit.col << " row: " << hit.row << " tot: " << hit.tot << "\n";
    //        std::cout << "TRUTH  : col: " << truth[ih].col + 1 << " row: " << truth[ih].row + 1 << " tot: " << truth[ih].tot - 1 << "\n";
    //        ih++;
    //    }
    //    ie++;
    //}

    for (int ie = 0; ie < rawData.events.size(); ie++){
        std::cout << "Event " << ie << "\n";
        for(int ih = 0; ih < rawData.events[ie].hits.size(); ih++){
            std::cout << "DECODED: col: " << rawData.events[ie].hits[ih].col << " row: " << rawData.events[ie].hits[ih].row << " tot: " << rawData.events[ie].hits[ih].tot << "\n";
            std::cout << "TRUTH  : col: " << truthData.events[ie].hits[ih].col << " row: " << truthData.events[ie].hits[ih].row << " tot: " << truthData.events[ie].hits[ih].tot << "\n";
            if (rawData.events[ie].hits[ih].row != truthData.events[ie].hits[ih].row || rawData.events[ie].hits[ih].col != truthData.events[ie].hits[ih].col || rawData.events[ie].hits[ih].tot != truthData.events[ie].hits[ih].tot){
                std::cout << "Mismatch\n";
                //break;
            }
        }
    }



    
}