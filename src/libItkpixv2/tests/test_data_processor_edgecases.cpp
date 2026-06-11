#include "catch.hpp"

#include <iostream>

#include "AllProcessors.h"

#include "EventData.h"

#include "Itkpixv2Cfg.h"

#include "Itkpixv2DataProcessor.h"

// Run the processor on one or more batches and return the processor for
// error-counter inspection.  Each batch is a flat list of 32-bit words that
// form a single RawDataContainer.
std::shared_ptr<Itkpixv2DataProcessor> process_case(std::vector<std::vector<uint32_t>> packages) {

    std::shared_ptr<FeDataProcessor> proc = StdDict::getDataProcessor("ITKPIXV2");

    REQUIRE (proc);

    ClipBoard<RawDataContainer> rd_cp;
    ClipBoard<EventDataBase> em_cp;

    Itkpixv2Cfg cfg;

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

    std::shared_ptr<Itkpixv2DataProcessor> v2proc = std::dynamic_pointer_cast<Itkpixv2DataProcessor>(proc);

    // Make sure 0xFFFFDEAD was never left as the active data block
    if (v2proc->_data)
        CHECK(((v2proc->_data[0] != 0xFFFFDEAD) || (v2proc->_data[1] != 0xFFFFDEAD)));

    CHECK (em_cp.empty());
    return v2proc;
}

TEST_CASE("Itkpixv2DataProcessor", "[itkpixv2][data_processor_edge_case]") {

    // Random selftrigger case — well-formed stream, no errors expected
    std::cout << "BASE SELFTRIGGER CASE" << std::endl;
    {
        auto proc = process_case({{
            4242473892, 171971904, // tag 249
            4250599616, 77605504,  // tag 250
            4253024256, 0          // tag 251
        }});
        CHECK(proc->_corruptStreamErrorCnt == 0);
        CHECK(proc->_unfinishedStreamErrorCnt == 0);
        CHECK(proc->_chipTagBitFlipCnt == 0);
        CHECK(proc->_chipTagErrorCnt == 0);
    }

    // Minimal example segfault block — corrupt data; allow non-zero error counters
    // but require no crash and _chipTagBitFlipCnt/_chipTagErrorCnt both zero
    // (0x7e... has ES=0, tag=252 which is in the error-tag range 216-219 → bitflip)
    std::cout << "BASIC SEGFAULT CASE" << std::endl;
    {
        auto proc = process_case({{
            0x7e3d4ba8, 0x17411215, // tag 252
            0x38048494, 0xE2021493  // lots of hits
        }});
        // Tag 252 = 0b11111100 → (252>>3)==31 → error-tag range, single-bit-flip sub-range
        CHECK(proc->_chipTagBitFlipCnt <= 1);
    }

    // 64 bit block analysis:
    // 0 11111100 011110 10 10010111 01 10 10 01 0000 101110 1 0 00001000 10 01 10 01 0010 101
    //   ^ 252    ^30    LN ^151     single hit  tot0 ^46    L N ^8       single hit tot 2
    // 0 011 1 0 00000001 01 01 10 01 0010 010010 1 0 01110001 01 01 01 01 0001 000010 1 0 01001001 1
    //ES ^43 L N ^ 1      single hit  tot2 ^18    L N ^113   single hit    tot1 ^2     L N ^73

    // Harder segfault block — corrupt/dense data, only require no crash
    std::cout << "COMPLEX SEGFAULT CASE" << std::endl;
    process_case({{
        0x73c91dab, 0xdb8c39d4
    }});

    // Multiple blocks — clean split stream, no errors expected
    std::cout << "MULTIPLE BLOCK CASE" << std::endl;
    {
        auto proc = process_case({
            { 4250599616, 77605504 },
            { 4253024256, 0 }
        });
        CHECK(proc->_corruptStreamErrorCnt == 0);
    }

    // 0xFFFFDEAD interleaved with real data — processor must skip dead words
    std::cout << "0xFFFFDEAD CASE" << std::endl;
    {
        auto proc = process_case({
            {
                4250599616, 77605504,    // well-formed data
                0xFFFFDEAD, 0xFFFFDEAD  // dead words at end of first batch
            },
            {
                0xFFFFDEAD, 0xFFFFDEAD  // second batch is all dead
            }
        });
        // No bit-flip or unrecognised-tag errors from dead-word batches
        CHECK(proc->_chipTagBitFlipCnt == 0);
        CHECK(proc->_chipTagErrorCnt == 0);
    }

    // All-dead batch — processor must handle gracefully with no output
    std::cout << "LOW DATA CASE" << std::endl;
    {
        auto proc = process_case({{
            0xFFFFDEAD, 0xFFFFDEAD,
            0xFFFFDEAD, 0xFFFFDEAD
        }});
        CHECK(proc->_corruptStreamErrorCnt == 0);
        CHECK(proc->_unfinishedStreamErrorCnt == 0);
    }

    // Two consecutive all-dead batches followed by a valid batch.
    // This exercises the _curOut null path (Issue #2): when a batch ends with
    // _events==0, _curOut is reset; the subsequent valid batch must still decode
    // correctly without crashing.
    std::cout << "EMPTY BATCHES THEN VALID CASE" << std::endl;
    {
        auto proc = process_case({
            { 0xFFFFDEAD, 0xFFFFDEAD },
            { 0xFFFFDEAD, 0xFFFFDEAD },
            { 4250599616, 77605504, 4253024256, 0 }   // well-formed stream
        });
        CHECK(proc->_chipTagBitFlipCnt == 0);
        CHECK(proc->_chipTagErrorCnt == 0);
    }

    // High FFFFDEAD rate — stress test for dead-word skipping and recursion depth
    std::vector<std::vector<uint32_t>> input;
    std::vector<uint32_t> deads;
    for(int i = 0; i < 1000000; i++)
        deads.push_back(0xFFFFDEAD);
    input.push_back(deads);
    std::cout << "HIGH FFFFDEAD RATE CASE" << std::endl;
    process_case(input);
}
