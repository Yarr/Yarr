#include "catch.hpp"

#include <iostream>

#include "AllProcessors.h"

#include "EventData.h"

#include "Rd53bCfg.h"

#include "Rd53bDataProcessor.h"

void process_case(std::vector<std::vector<uint32_t>> packages) {
  
    std::shared_ptr<FeDataProcessor> proc = StdDict::getDataProcessor("RD53B");

    REQUIRE (proc);

    ClipBoard<RawDataContainer> rd_cp;
    ClipBoard<EventDataBase> em_cp;

    Rd53bCfg cfg;  

    proc->connect(&cfg, &rd_cp, &em_cp );
    proc->init();
    proc->run();


    for (auto words : packages) {

        int nWords = words.size();
        std::unique_ptr<RawDataContainer> rdc(new RawDataContainer(LoopStatus()));
        RawDataPtr rd = std::make_shared<RawData>(0, nWords);
        uint32_t *buffer = rd->getBuf();
        buffer[nWords-1] = 0;
        std::copy(words.data(), words.data()+nWords, buffer);

        rdc->add(std::move(rd));
        rd_cp.pushData(std::move(rdc));
    }

    rd_cp.finish();
    proc->join();

    while(!em_cp.empty()) {
        auto data = em_cp.popData();
        FrontEndData &rawData = *(FrontEndData*)data.get();
        REQUIRE(rawData.events.size() > 0);
    }

    // Make sure FFFFDEAD was never a processed data block
    std::shared_ptr<Rd53bDataProcessor> v1proc = std::dynamic_pointer_cast<Rd53bDataProcessor>(proc);
    if(v1proc->_data)
        REQUIRE(((v1proc->_data[0] != 0xFFFFDEAD) && (v1proc->_data[1] != 0xFFFFDEAD)));
    
    // Only one thing
    REQUIRE (em_cp.empty());
}

TEST_CASE("Rd53bDataProcessor", "[rd53b][data_processor_edgecases]") {
    
    // Streams of all zeros
    std::cout << "ZERO STREAM CASE" << std::endl;
    process_case({{0, 0, 0, 0}});

    // Basic segfault edge case (see libItkpixv2 tests for detailed analysis)
    std::cout << "BASIC SEGFAULT CASE" << std::endl;
    process_case({{
        0xfe3d4ba8, 0x17411215,
        0x38048494, 0xe2021493
    }});

    // Edge case for harder segfault 
    std::cout << "COMPLEX SEGFAULT CASE" << std::endl;
    process_case({{0xF3C91DAB, 0xDB8C39D4}});

    // 0xffffdead case
    std::cout << "0xFFFFDEAD CASE" << std::endl;
    process_case({
        {
            4250599616, 77605504,       // Fully complete data
            0xFFFFDEAD, 0xFFFFDEAD      // ffffdead at end of block
        },
        {
            0xFFFFDEAD, 0xFFFFDEAD      // This will leave data processor in a state where last block is 0xFFFFDEAD
        }
    });

    // Low data case
    std::cout << "LOW DATA CASE" << std::endl;
    process_case({
        {
            0xFFFFDEAD, 0xFFFFDEAD,     // Lots of FFFFDEAD
            0xFFFFDEAD, 0xFFFFDEAD      // FFFFDEADs at end of block
        }
    });

    // High FFFFDEAD rate case
    std::vector<std::vector<uint32_t>> input;
    std::vector<uint32_t> deads;
    for(int i = 0; i < 1000000; i++)
        deads.push_back(0xFFFFDEAD);
    input.push_back(deads);
    std::cout << "HIGH FFFFDEAD RATE CASE" << std::endl;
    process_case(
        input
    );
}
