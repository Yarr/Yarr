#include "catch.hpp"

#include "Itkpixv2Encoder.h"
#include <iostream>
#include <memory>
#include <bitset>

#include "AllProcessors.h"

#include "EventData.h"

#include "HitMapGenerator.h"

#include "Itkpixv2Cfg.h"
#include "Rd53bCfg.h"



int main(){
    FrontEndData truth;
    
    std::unique_ptr<HitMapGenerator> generator(new HitMapGenerator());
    int nEvents = 1;
    int nEventsPerStream = 5;
    
    std::unique_ptr<Itkpixv2Encoder> encoder(new Itkpixv2Encoder());
    encoder->setEventsPerStream(nEventsPerStream);

    for (int evt = 0; evt < nEvents; evt++){
        generator->randomHitMap();
        truth.events.push_back(generator->outTruth());
        if   (evt != nEvents - 1) encoder->addToStream(generator->outHits(), false);
        else                      encoder->addToStream(generator->outHits(), true); //make sure the stream is ended with the last added event
    }
    

    std::vector<uint32_t> words = encoder->getWords();

    for (auto& w : words){
        std::bitset<32> bw(w);
        std::cout << bw << "\n";
    }
    
    
    int nWords = words.size();
    std::cout << "Helo\n";
    //std::shared_ptr<FeDataProcessor> proc = StdDict::getDataProcessor("ITKPIXV2");
    std::shared_ptr<FeDataProcessor> proc = StdDict::getDataProcessor("RD53B");

    ClipBoard<RawDataContainer> rd_cp;
    ClipBoard<EventDataBase> em_cp;

    //Itkpixv2Cfg cfg;  
    Rd53bCfg cfg;
    proc->connect(&cfg, &rd_cp, &em_cp );

    proc->init();
    proc->run();
    RawDataPtr rd = std::make_shared<RawData>(0, nWords);
    uint32_t *buffer = rd->getBuf();
    buffer[nWords-1] = 0;

    std::copy(words.data(), words.data()+nWords, buffer);
    std::unique_ptr<RawDataContainer> rdc(new RawDataContainer(LoopStatus()));

    rdc->add(std::move(rd));

    rd_cp.pushData(std::move(rdc));

    rd_cp.finish();
    std::cout << "Now, it will fail in TEST_CASE\n";
    proc->join();
    std::cout << "But not in a standalone exe\n";
    auto data = em_cp.popData();
    FrontEndData &rawData = *(FrontEndData*)data.get();

    int truthNHits = 0;
    int rawNHits = 0;
    
    #if 0 //enable to see the truth vs. decoded comparison
    for (int ievt = 0; ievt < rawData.events.size(); ievt++){
        std::cout << "EVENT TAG = " << rawData.events[ievt].tag << "\n";
	    for(int ihit = 0; ihit < rawData.events[ievt].hits.size(); ihit++){
            std::cout << "HIT " << ihit << ":\n";
            std::cout << "Dec col = " << rawData.events[ievt].hits[ihit].col << "   truth col = " << truth.events[ievt].hits[ihit].col << "\n";
		    std::cout << "Dec row = " << rawData.events[ievt].hits[ihit].row << "   truth row = " << truth.events[ievt].hits[ihit].row << "\n";
		    std::cout << "Dec tot = " << rawData.events[ievt].hits[ihit].tot << "   truth tot = " << truth.events[ievt].hits[ihit].tot << "\n";
            rawNHits++;
	    }
    }
    #endif

}

