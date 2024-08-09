#include "catch.hpp"

#include "Itkpixv2Encoder.h"
#include <iostream>
#include <bitset>

#include "AllProcessors.h"

#include "EventData.h"

#include "HitMapGenerator.h"

#include "Itkpixv2Cfg.h"
#include "Itkpixv2DataProcessor.h"



TEST_CASE("Itkpixv2DataProcessor", "[itkpixv2][error_tags]") {
    FrontEndData truth;
    
    std::unique_ptr<HitMapGenerator> generator(new HitMapGenerator());
    int nEvents = 250;
    int nEventsPerStream = 1;
    generator->setSeed(Catch::rngSeed());
    
    std::unique_ptr<Itkpixv2Encoder> encoder(new Itkpixv2Encoder());
    encoder->setEventsPerStream(nEventsPerStream);

    for (int evt = 0; evt < nEvents; evt++){
        generator->randomHitMap(1e-4);
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

    for (int ievt = 0; ievt < rawData.events.size(); ievt++){
	    for(int ihit = 0; ihit < rawData.events[ievt].hits.size(); ihit++){
            REQUIRE(rawData.events[ievt].hits[ihit].col == truth.events[ievt].hits[ihit].col);
		    REQUIRE(rawData.events[ievt].hits[ihit].row == truth.events[ievt].hits[ihit].row);
		    REQUIRE(rawData.events[ievt].hits[ihit].tot == truth.events[ievt].hits[ihit].tot);
            rawNHits++;
	    }
        truthNHits += truth.events[ievt].nHits;
    }

    unsigned bitFlipCnt = 4, errorTagCnt = 4;

    Itkpixv2DataProcessor* proc_raw = dynamic_cast<Itkpixv2DataProcessor*>(proc.get());
    REQUIRE(rawNHits == truthNHits);

    REQUIRE(bitFlipCnt == proc_raw->_chipTagBitFlipCnt);
    REQUIRE(errorTagCnt == proc_raw->_chipTagErrorCnt);
}