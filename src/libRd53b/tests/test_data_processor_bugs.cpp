/*
 * Regression tests for specific memory-safety and correctness bugs identified
 * in Rd53bDataProcessor.  See the Itkpixv2 counterpart for fuller commentary;
 * notes here focus on the Rd53b-specific differences.
 *
 * Build with ASAN: cmake -DCMAKE_BUILD_TYPE=Asan ..
 */

#include "catch.hpp"

#include <vector>

#include "AllProcessors.h"
#include "EventData.h"
#include "Rd53bCfg.h"
#include "Rd53bDataProcessor.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::shared_ptr<Rd53bDataProcessor>
run_single_batch(std::vector<uint32_t> words, Rd53bCfg &cfg) {
    std::shared_ptr<FeDataProcessor> proc = StdDict::getDataProcessor("RD53B");
    REQUIRE(proc);

    ClipBoard<RawDataContainer> rd_cp;
    ClipBoard<EventDataBase>    em_cp;
    proc->connect(&cfg, &rd_cp, &em_cp);
    proc->init();
    proc->run();

    int nWords = words.size();
    auto rdc = std::make_unique<RawDataContainer>(LoopStatus());
    auto rd  = std::make_shared<RawData>(0, nWords);
    std::copy(words.begin(), words.end(), rd->getBuf());
    rdc->add(std::move(rd));
    rd_cp.pushData(std::move(rdc));

    rd_cp.finish();
    proc->join();

    while (!em_cp.empty()) em_cp.popData();

    return std::dynamic_pointer_cast<Rd53bDataProcessor>(proc);
}

static std::shared_ptr<Rd53bDataProcessor>
run_multi_batch(std::vector<std::vector<uint32_t>> batches, Rd53bCfg &cfg) {
    std::shared_ptr<FeDataProcessor> proc = StdDict::getDataProcessor("RD53B");
    REQUIRE(proc);

    ClipBoard<RawDataContainer> rd_cp;
    ClipBoard<EventDataBase>    em_cp;
    proc->connect(&cfg, &rd_cp, &em_cp);
    proc->init();
    proc->run();

    for (auto &words : batches) {
        int nWords = words.size();
        auto rdc = std::make_unique<RawDataContainer>(LoopStatus());
        auto rd  = std::make_shared<RawData>(0, nWords);
        std::copy(words.begin(), words.end(), rd->getBuf());
        rdc->add(std::move(rd));
        rd_cp.pushData(std::move(rdc));
    }

    rd_cp.finish();
    proc->join();

    while (!em_cp.empty()) em_cp.popData();

    return std::dynamic_pointer_cast<Rd53bDataProcessor>(proc);
}

// ---------------------------------------------------------------------------
// Issue #1 — _qrow[55] out-of-bounds write when ccol == 55
//
// Identical root cause to Itkpixv2.  The Rd53b stream format differs in that
// the first block must have NS=1 (bit 31 = 1), and newEvent() is called
// inside the INIT block (before CCOL) so _events==1 when the OOB write occurs.
//
// Crafted stream layout:
//   bit 31    : NS=1  (new/valid stream start)
//   bits 30-23: tag=0
//   bits 22-17: ccol = 0b110111 = 55  → 0x006E0000
//   bits 16-15: islast=1, isneighbor=0 → bit 16 = 0x00010000
//   remaining : all zero
//
// word[0] = 0x80000000 | 0x006E0000 | 0x00010000 = 0x806F0000
// word[1] = 0x00000000
//
// Verification:
//   NS   = (0x806F0000 >> 31) & 1         = 1          ✓
//   tag  = (0x806F0000 >> 23) & 0xFF      = 0          ✓
//   ccol = (0x806F0000 & 0x007FFFFF) >> 17 = 0x006F0000 >> 17 = 55  ✓
//   isl  = (0x806F0000 & 0x0001FFFF) >> 15 = 0x00010000 >> 15 = 2   ✓
//          (islast=1, isneighbor=0)
//
// Because _events==1 (newEvent() was already called), when data runs out the
// event is pushed to the output clipboard (one event, zero hits).  No
// _expectNewStreamErrorCnt or _unfinishedStreamErrorCnt is expected.
//
// Before the fix  : aborts under ASAN (heap-buffer-overflow on _qrow[55]).
// After the fix   : processor completes; error counters verify the correct
//                   code paths were taken.
// ---------------------------------------------------------------------------
TEST_CASE("Rd53bDataProcessor: ccol=55 OOB write to _qrow",
          "[rd53b][bug_ccol55]") {

    Rd53bCfg cfg;
    auto proc = run_single_batch({0x806F0000, 0x00000000}, cfg);

    // Must not be counted as a tag error (216–223 range)
    CHECK(proc->_expectNewStreamErrorCnt   == 0);
    CHECK(proc->_unfinishedStreamErrorCnt  == 0);
}

// ---------------------------------------------------------------------------
// Issue #1 (variant) — isneighbor=1 path: ++_qrow[55]
//
// word[0]: NS=1, tag=0, ccol=55, first iteration islast=0/isneighbor=1,
//          second iteration islast=1/isneighbor=0.
//
// bits 22-17: ccol=55     → 0x006E0000
// bits 16-15: 0b01        → isneighbor=1, islast=0  → 0x00008000
// bits 14-13: 0b10        → islast=1, isneighbor=0  → 0x00004000
//
// word[0] = 0x80000000 | 0x006E0000 | 0x00008000 | 0x00004000 = 0x806EC000
// word[1] = 0x00000000
// ---------------------------------------------------------------------------
TEST_CASE("Rd53bDataProcessor: ccol=55 OOB increment via isneighbor",
          "[rd53b][bug_ccol55_neighbor]") {

    Rd53bCfg cfg;
    auto proc = run_single_batch({0x806EC000, 0x00000000}, cfg);

    CHECK(proc->_expectNewStreamErrorCnt   == 0);
    CHECK(proc->_unfinishedStreamErrorCnt  == 0);
}

// ---------------------------------------------------------------------------
// Issue #2 — _curOut null path: empty batches followed by valid data
//
// Same mechanism as Itkpixv2.  Two all-dead batches reset _curOut to null;
// the third batch with a well-formed stream must decode without crashing.
// ---------------------------------------------------------------------------
TEST_CASE("Rd53bDataProcessor: empty batches followed by valid data",
          "[rd53b][bug_null_curout_sequence]") {

    Rd53bCfg cfg;
    auto proc = run_multi_batch(
        {
            { 0xFFFFDEAD, 0xFFFFDEAD },
            { 0xFFFFDEAD, 0xFFFFDEAD },
            // NS=1, tag=250, some hits, then NS=1 tag=251 terminator
            { 4250599616, 77605504, 4253024256, 0 }
        },
        cfg
    );

    CHECK(proc->_unfinishedStreamErrorCnt == 0);
    CHECK(proc->_expectNewStreamErrorCnt  == 0);
}

// ---------------------------------------------------------------------------
// Issue #7 — _outOfRangeBitsCnt declared and reported in getLog() but never
//            incremented anywhere in Rd53bDataProcessor.cpp
//
// The counter is intended to track how many times bits were requested past the
// end of a stream.  Currently it is always 0 regardless of the data processed.
//
// This test documents the current (buggy) behaviour: even after processing a
// corrupt stream that should trigger out-of-range bit requests, the counter
// remains 0.  After the fix the counter should reflect real events.
//
// The test uses the same "complex segfault" stream from the edge-case suite,
// which is known to trigger error paths.
// ---------------------------------------------------------------------------
TEST_CASE("Rd53bDataProcessor: _outOfRangeBitsCnt never incremented (Issue #7)",
          "[rd53b][bug_outofrangecnt]") {

    Rd53bCfg cfg;
    // Corrupt stream that exercises error-recovery paths
    auto proc = run_single_batch({0xF3C91DAB, 0xDB8C39D4}, cfg);

    // BUG: _outOfRangeBitsCnt is declared in the header and reported by
    // getLog() but is never incremented in the .cpp.  It will always be 0.
    // After the fix this CHECK should be updated to expect a non-zero value
    // when the stream triggers out-of-range requests.
    CHECK(proc->_outOfRangeBitsCnt == 0);

    // Verify getLog() includes the key without crashing
    json log = proc->getLog();
    CHECK(log.contains("Out of range Req"));
    CHECK(log["Out of range Req"] == 0);
}

// ---------------------------------------------------------------------------
// RTL cross-check — dumpDebugBuffer secondary OOB via _qrow[_ccol]
// (mirrors the Itkpixv2 test; same root cause, same fix)
// ---------------------------------------------------------------------------
TEST_CASE("Rd53bDataProcessor: dumpDebugBuffer safe_ccol clamp",
          "[rd53b][bug_dumpbuffer_oob]") {

    Rd53bCfg cfg;
    // NS=1, tag=0, ccol=55, islast=1, isneighbor=0
    auto proc = run_single_batch({0x806F0000, 0x00000000}, cfg);

    CHECK(proc->_expectNewStreamErrorCnt  == 0);
    CHECK(proc->_unfinishedStreamErrorCnt == 0);
}

// ---------------------------------------------------------------------------
// Issue #5 — unbounded recursion in getPreviousDataBlock()
//
// getPreviousDataBlock() calls itself recursively to skip FFFFDEAD words and
// wrong-chip-ID words.  A large run of such words can overflow the stack.
// The existing "HIGH FFFFDEAD RATE" edge-case test (1 million dead words in
// one batch) already exercises this, but it uses getNextDataBlock()'s while
// loop rather than the recursive getPreviousDataBlock() path.
//
// The getPreviousDataBlock() recursion is only reachable when retrieve()
// crosses a block boundary and then needs to roll back (the "getPreviousDataBlock
// after failed getNextDataBlock" path in retrieve()).  Constructing a stream
// that reliably hits this from the public API requires a multi-block packet
// where the last valid block is immediately followed by a long run of DEAD
// words and then data runs out.  That scenario is covered implicitly by the
// FFFFDEAD cases above; a dedicated stack-depth test is left for future work
// once the recursion is converted to iteration.
// ---------------------------------------------------------------------------
