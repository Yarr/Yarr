#include "catch.hpp"

#include <iostream>

#include "AllProcessors.h"

#include "EventData.h"

#include "Rd53bCfg.h"

void process_case(std::vector<uint32_t> words) {
    int nWords = words.size();
  
    std::shared_ptr<FeDataProcessor> proc = StdDict::getDataProcessor("RD53B");

    REQUIRE (proc);

    ClipBoard<RawDataContainer> rd_cp;
    ClipBoard<EventDataBase> em_cp;

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

    proc->join();

    if(!em_cp.empty()) {
        auto data = em_cp.popData();
        FrontEndData &rawData = *(FrontEndData*)data.get();
        REQUIRE(rawData.events.size() > 0);
    }
    // Only require that data has been processed
    REQUIRE (em_cp.empty());
}

TEST_CASE("Rd53bDataProcessor", "[rd53b][data_processor_edgecases]") {
    
    // Streams of all zeros
    process_case({0, 0, 0, 0});

    // Basic segfault edge case (see libItkpixv2 tests for detailed analysis)
    process_case({
        0xfe3d4ba8, 0x17411215,
        0x38048494, 0xe2021493
    });

    // Edge case for harder segfault 
    process_case({0xF3C91DAB, 0xDB8C39D4});
}
