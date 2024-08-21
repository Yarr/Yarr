#include "catch.hpp"

#include <iostream>

#include "AllProcessors.h"

#include "EventData.h"

#include "Itkpixv2Cfg.h"

void process_case(std::vector<uint32_t> words) {
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

    if(!em_cp.empty()) {
        auto data = em_cp.popData();
        FrontEndData &rawData = *(FrontEndData*)data.get();
        REQUIRE(rawData.events.size() > 0);
    }
    
    // Only one thing
    REQUIRE (em_cp.empty());
}

TEST_CASE("Itkpixv2DataProcessor", "[itkpixv2][data_processor_edge_case]") {

    // Random selftrigger case
    process_case({
        4242473892, 171971904, // tag 249
        4250599616, 77605504,  // tag 250
        4253024256, 0          // tag 251
    });

    // Minimal example segfault block
    process_case({
        0x7e3d4ba8, 0x17411215, // tag 252
        0x38048494, 0xE2021493  // lots of hits
    });

    // 64 bit block analysis:
    // 0 11111100 011110 10 10010111 01 10 10 01 0000 101110 1 0 00001000 10 01 10 01 0010 101                  // 0 11111100 011110 1 0 10010111 0 10100000010111010000010001001000010101
    //   ^ 252    ^30    LN ^151     single hit  tot0 ^46    L N ^8       single hit tot 2 

    // 0 011 1 0 00000001 01 01 10 01 0010 010010 1 0 01110001 01 01 01 01 0001 000010 1 0 01001001 1
    //ES ^43 L N ^ 1      single hit  tot2 ^18    L N ^113   single hit    tot1 ^2     L N ^73      ^ this is removing a segfault
    
    // Harder segfault block
    process_case({
        0x73c91dab, 0xdb8c39d4
    });
}
