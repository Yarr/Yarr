#include "catch.hpp"

#include "Itkpixv2Encoder.h"
#include <iostream>
#include <bitset>

#include "AllProcessors.h"

#include "EventData.h"

#include "HitMapGenerator.h"

#include "Itkpixv2Cfg.h"

#include "Rd53bCfg.h"



TEST_CASE("Itkpixv2DataProcessor", "[itkpixv2][data_processor]") {
    FrontEndData truth;
    
    std::unique_ptr<HitMapGenerator> generator(new HitMapGenerator());
    int nEvents = 2;
    int nEventsPerStream = 16;
    generator->setSeed(Catch::rngSeed());
    
    std::unique_ptr<Itkpixv2Encoder> encoder(new Itkpixv2Encoder());
    encoder->setEventsPerStream(nEventsPerStream);

    for (int evt = 0; evt < nEvents; evt++){
        generator->randomHitMap(1e-2);
        truth.events.push_back(generator->outTruth());
        if   (evt != nEvents - 1) encoder->addToStream(generator->outHits());
        else                      encoder->addToStream(generator->outHits(), true); //make sure the stream is ended with the last added event
    }
    

    std::vector<uint32_t> words = encoder->getWords();

    int nWords = words.size();

    std::shared_ptr<FeDataProcessor> proc = StdDict::getDataProcessor("ITKPIXV2");
    REQUIRE (proc);
    ClipBoard<RawDataContainer> rd_cp;
    ClipBoard<EventDataBase> em_cp;

    Itkpixv2Cfg cfg;  
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

    proc->join();
    REQUIRE (!em_cp.empty());

    auto data = em_cp.popData();
    FrontEndData &rawData = *(FrontEndData*)data.get();

    int truthNHits = 0;
    int rawNHits = 0;


    #if 1
    for (int ievt = 0; ievt < rawData.events.size(); ievt++){
        std::cout << "TRUTH EVENT WITH " << truth.events[ievt].hits.size() << " HITS\n";
	    for(int ihit = 0; ihit < rawData.events[ievt].hits.size(); ihit++){
            std::cout << "HIT " << ihit << " out of " << rawData.events[ievt].hits.size() << ":\n";
            std::cout << "Dec col = " << rawData.events[ievt].hits[ihit].col << "   truth col = " << truth.events[ievt].hits[ihit].col << "\n";
		    std::cout << "Dec row = " << rawData.events[ievt].hits[ihit].row << "   truth row = " << truth.events[ievt].hits[ihit].row << "\n";
		    std::cout << "Dec tot = " << rawData.events[ievt].hits[ihit].tot << "   truth tot = " << truth.events[ievt].hits[ihit].tot << "\n";
            REQUIRE(rawData.events[ievt].hits[ihit].col == truth.events[ievt].hits[ihit].col);
		    REQUIRE(rawData.events[ievt].hits[ihit].row == truth.events[ievt].hits[ihit].row);
		    REQUIRE(rawData.events[ievt].hits[ihit].tot == truth.events[ievt].hits[ihit].tot);
            if (rawData.events[ievt].hits[ihit].tot != truth.events[ievt].hits[ihit].tot){std::cout << "fuuuck\n"; break;}
            rawNHits++;
	    }
        if (rawData.events[ievt].hits.size() != truth.events[ievt].hits.size()){
            std::cout << "Undecoded hit: truth col = " << truth.events[ievt].hits[rawData.events[ievt].hits.size()].col << "\n";
		    std::cout << "Undecoded hit: truth row = " << truth.events[ievt].hits[rawData.events[ievt].hits.size()].row << "\n";
		    std::cout << "Undecoded hit: truth tot = " << truth.events[ievt].hits[rawData.events[ievt].hits.size()].tot << "\n";
            std::cout << "few last words:\n";
            for (auto& w : words){//(int i = words.size() - 5; i < words.size(); i++){
                std::bitset<32> bw(w);
                std::cout << bw.to_string() << "\n";
            }
        }
        truthNHits += truth.events[ievt].nHits;
    }

            //for (auto& w : words){//(int i = words.size() - 5; i < words.size(); i++){
            //    std::bitset<32> bw(w);
            //    std::cout << bw.to_string() << "\n";
            //}


    REQUIRE(rawNHits == truthNHits);
    #endif
    #if 0 //enable to see the truth vs. decoded comparison
    for (int ievt = 0; ievt < rawData.events.size(); ievt++){
        std::cout << "EVENT TAG = " << rawData.events[ievt].bcid << "\n";
	    for(int ihit = 0; ihit < rawData.events[ievt].hits.size(); ihit++){
            std::cout << "HIT " << ihit << ":\n";
            std::cout << "Dec col = " << rawData.events[ievt].hits[ihit].col << "   truth col = " << truth.events[ievt].hits[ihit].col << "\n";
		    std::cout << "Dec row = " << rawData.events[ievt].hits[ihit].row << "   truth row = " << truth.events[ievt].hits[ihit].row << "\n";
		    std::cout << "Dec tot = " << rawData.events[ievt].hits[ihit].tot << "   truth tot = " << truth.events[ievt].hits[ihit].tot << "\n";
            if (rawData.events[ievt].hits[ihit].tot != truth.events[ievt].hits[ihit].tot){std::cout << "fuuuck\n"; break;}
            rawNHits++;
	    }
    }
    #endif

}

