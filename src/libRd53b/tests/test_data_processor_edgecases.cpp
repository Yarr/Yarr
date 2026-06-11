#include "catch.hpp"

#include <iostream>

#include "AllProcessors.h"

#include "EventData.h"

#include "Rd53bCfg.h"

#include "Rd53bDataProcessor.h"

// Run the processor on one or more batches and return the processor for
// error-counter inspection.
std::shared_ptr<Rd53bDataProcessor> process_case(std::vector<std::vector<uint32_t>> packages) {

    std::shared_ptr<FeDataProcessor> proc = StdDict::getDataProcessor("RD53B");

    REQUIRE (proc);

    ClipBoard<RawDataContainer> rd_cp;
    ClipBoard<EventDataBase> em_cp;

    Rd53bCfg cfg;

    proc->connect(&cfg, &rd_cp, &em_cp );
    proc->init();
    proc->run();

    for (auto& words : packages) {
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
        CHECK(rawData.events.size() > 0);
    }

    std::shared_ptr<Rd53bDataProcessor> v1proc = std::dynamic_pointer_cast<Rd53bDataProcessor>(proc);

    // Make sure 0xFFFFDEAD was never left as the active data block
    if (v1proc->_data)
        CHECK(((v1proc->_data[0] != 0xFFFFDEAD) || (v1proc->_data[1] != 0xFFFFDEAD)));

    CHECK (em_cp.empty());
    return v1proc;
}

TEST_CASE("Rd53bDataProcessor", "[rd53b][data_processor_edgecases]") {

    // All-zeros stream — NS=0 on first block triggers _expectNewStreamErrorCnt
    std::cout << "ZERO STREAM CASE" << std::endl;
    {
        auto proc = process_case({{0, 0, 0, 0}});
        CHECK(proc->_expectNewStreamErrorCnt >= 1);
    }

    // Basic segfault edge case — corrupt data, only require no crash
    std::cout << "BASIC SEGFAULT CASE" << std::endl;
    process_case({{
        0xfe3d4ba8, 0x17411215,
        0x38048494, 0xe2021493
    }});

    // Harder segfault block — dense data, only require no crash
    std::cout << "COMPLEX SEGFAULT CASE" << std::endl;
    process_case({{0xF3C91DAB, 0xDB8C39D4}});

    // 0xFFFFDEAD interleaved with real data
    std::cout << "0xFFFFDEAD CASE" << std::endl;
    {
        auto proc = process_case({
            {
                4250599616, 77605504,    // well-formed data
                0xFFFFDEAD, 0xFFFFDEAD
            },
            {
                0xFFFFDEAD, 0xFFFFDEAD
            }
        });
        CHECK(proc->_unfinishedStreamErrorCnt == 0);
    }

    // All-dead batch — no output, no stream errors
    std::cout << "LOW DATA CASE" << std::endl;
    {
        auto proc = process_case({{
            0xFFFFDEAD, 0xFFFFDEAD,
            0xFFFFDEAD, 0xFFFFDEAD
        }});
        CHECK(proc->_unfinishedStreamErrorCnt == 0);
    }

    // Two consecutive all-dead batches followed by a valid batch (Issue #2 path)
    std::cout << "EMPTY BATCHES THEN VALID CASE" << std::endl;
    {
        auto proc = process_case({
            { 0xFFFFDEAD, 0xFFFFDEAD },
            { 0xFFFFDEAD, 0xFFFFDEAD },
            { 4250599616, 77605504, 4253024256, 0 }
        });
        CHECK(proc->_unfinishedStreamErrorCnt == 0);
    }

    // High FFFFDEAD rate — stress test
    std::vector<std::vector<uint32_t>> input;
    std::vector<uint32_t> deads;
    for(int i = 0; i < 1000000; i++)
        deads.push_back(0xFFFFDEAD);
    input.push_back(deads);
    std::cout << "HIGH FFFFDEAD RATE CASE" << std::endl;
    process_case(input);
}
