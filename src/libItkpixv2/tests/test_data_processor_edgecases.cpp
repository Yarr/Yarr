#include "catch.hpp"

#include <iostream>

#include "AllProcessors.h"

#include "EventData.h"

#include "Itkpixv2Cfg.h"

#include "Itkpixv2DataProcessor.h"

void process_case(std::vector<std::vector<uint32_t>> packages) {
  
    std::shared_ptr<FeDataProcessor> proc = StdDict::getDataProcessor("ITKPIXV2");

    REQUIRE (proc);

    ClipBoard<RawDataContainer> rd_cp;
    ClipBoard<EventDataBase> em_cp;

    Itkpixv2Cfg cfg;  

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
    std::shared_ptr<Itkpixv2DataProcessor> v2proc = std::dynamic_pointer_cast<Itkpixv2DataProcessor>(proc);
    if(v2proc->_data)
        REQUIRE(((v2proc->_data[0] != 0xFFFFDEAD) && (v2proc->_data[1] != 0xFFFFDEAD)));
    
    // Only one thing
    REQUIRE (em_cp.empty());
}

TEST_CASE("Itkpixv2DataProcessor", "[itkpixv2][data_processor_edge_case]") {

    // Random selftrigger case
    std::cout << "BASE SELFTRIGGER CASE" << std::endl;
    process_case({{
        4242473892, 171971904, // tag 249
        4250599616, 77605504,  // tag 250
        4253024256, 0          // tag 251
    }});

    // Minimal example segfault block
    std::cout << "BASIC SEGFAULT CASE" << std::endl;
    process_case({{
        0x7e3d4ba8, 0x17411215, // tag 252
        0x38048494, 0xE2021493  // lots of hits
    }});

    // 64 bit block analysis:
    // 0 11111100 011110 10 10010111 01 10 10 01 0000 101110 1 0 00001000 10 01 10 01 0010 101                  // 0 11111100 011110 1 0 10010111 0 10100000010111010000010001001000010101
    //   ^ 252    ^30    LN ^151     single hit  tot0 ^46    L N ^8       single hit tot 2 

    // 0 011 1 0 00000001 01 01 10 01 0010 010010 1 0 01110001 01 01 01 01 0001 000010 1 0 01001001 1
    //ES ^43 L N ^ 1      single hit  tot2 ^18    L N ^113   single hit    tot1 ^2     L N ^73      ^ this is removing a segfault
    
    // Harder segfault block
    std::cout << "COMPLEX SEGFAULT CASE" << std::endl;
    process_case({{
        0x73c91dab, 0xdb8c39d4
    }});

    // Multiple blocks
    std::cout << "MULTIPLE BLOCK CASE" << std::endl;
    process_case({
        {
            4250599616, 77605504
        },
        {
            4253024256, 0
        }
    });

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
