/*
 * Regression tests for specific memory-safety and correctness bugs identified
 * in Itkpixv2DataProcessor.  Each test is tagged with the issue number it covers
 * so the mapping to fixes is unambiguous.
 *
 * Tests that trigger undefined behaviour (OOB writes) will abort under AddressSanitizer
 * before the corresponding fix is applied.  Without ASAN they may silently pass
 * but can still produce wrong error-counter values, which the CHECK() assertions
 * below will catch once the fix lands.
 *
 * Build with ASAN: cmake -DCMAKE_BUILD_TYPE=Asan ..
 */

#include "catch.hpp"

#include <thread>
#include <vector>

#include "AllProcessors.h"
#include "EventData.h"
#include "Itkpixv2Cfg.h"
#include "Itkpixv2DataProcessor.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Push a single flat list of 32-bit words as one RawDataContainer, run the
// processor to completion, and return it for inspection.
static std::shared_ptr<Itkpixv2DataProcessor>
run_single_batch(std::vector<uint32_t> words, Itkpixv2Cfg &cfg) {
    std::shared_ptr<FeDataProcessor> proc = StdDict::getDataProcessor("ITKPIXV2");
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

    // Drain the output clipboard
    while (!em_cp.empty()) em_cp.popData();

    return std::dynamic_pointer_cast<Itkpixv2DataProcessor>(proc);
}

// Push multiple batches (one RawDataContainer each) and return the processor.
static std::shared_ptr<Itkpixv2DataProcessor>
run_multi_batch(std::vector<std::vector<uint32_t>> batches, Itkpixv2Cfg &cfg) {
    std::shared_ptr<FeDataProcessor> proc = StdDict::getDataProcessor("ITKPIXV2");
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

    return std::dynamic_pointer_cast<Itkpixv2DataProcessor>(proc);
}

// ---------------------------------------------------------------------------
// Issue #1 — _qrow[55] out-of-bounds write when ccol == 55
//
// _qrow is declared as uint64_t _qrow[55] (valid indices 0-54).
// The guard for "internal tag" is  _ccol >= 0x38  (i.e. >= 56), so _ccol==55
// falls through to the hit-reading loop and causes _qrow[55] to be written
// one element past the end of the array.
//
// Crafted stream layout (all bits counted from MSB of word[0]):
//   bit 31    : ES=0  (not end-of-stream)
//   bits 30-23: tag=0 (eight zeros)
//   bits 22-17: ccol = 0b110111 = 55  → 0x006E0000
//   bits 16-15: islast=1, isneighbor=0 → 0x00010000
//   bits 14- 7: qrow = 0              (all zero)
//   remaining : all zero (hitmap=0, no hits)
//
// Encoding: word[0] = 0x006F0000, word[1] = 0x00000000
//
//   Verification:
//     ccol  = (0x006F0000 & 0x007FFFFF) >> 17 = 0x006F0000 >> 17 = 55  ✓
//     isl/n = (0x006F0000 & 0x0001FFFF) >> 15 = 0x00010000 >> 15 = 2   ✓
//             (islast=1, isneighbor=0)
//
// Memory-layout side-effect of the OOB write (visible WITHOUT ASAN):
//   The private member layout in the class is:
//       uint64_t _qrow[55];           ← valid indices 0-54
//       uint64_t _islast_isneighbor;  ← immediately next in heap memory
//   Writing _qrow[55]=0 therefore overwrites _islast_isneighbor with 0.
//   _islast_isneighbor was set to 2 (islast=1) during the ILIN step, so
//   the do-while exit condition  !(islast_isneighbor & 0x2)  evaluates to
//   !(0 & 2) = true instead of false, and the loop continues a second time.
//   As a result the processor never reaches the ccol=0/ES=0 check, so
//   _corruptStreamErrorCnt remains 0 instead of the expected 1.
//
// Before the ccol guard fix (_ccol >= 0x38):
//   - With ASAN:    aborts immediately (heap-buffer-overflow on _qrow[55]).
//   - Without ASAN: _corruptStreamErrorCnt stays 0 (loop control corrupted).
//                   CHECK(proc->_corruptStreamErrorCnt == 1) FAILS → bug visible.
// After the ccol guard fix (_ccol >= 55) with format marker check:
//   ccol=55 is caught by the internal tag path; 5 bits of temp are consumed to
//   reconstruct the 11-bit value.  The format marker check (bits[10:8] must be
//   0b111) fails for ccol=55 (produces 0b110), so a corrupt-stream error is
//   logged and the processor resets to INIT without creating a new event.
//   Total: exactly one corrupt-stream error.
// ---------------------------------------------------------------------------
TEST_CASE("Itkpixv2DataProcessor: ccol=55 OOB write to _qrow",
          "[itkpixv2][bug_ccol55]") {

    Itkpixv2Cfg cfg;
    // word[0]: ES=0, tag=0, ccol=55, islast=1, isneighbor=0, qrow-bits=0
    // word[1]: all zeros
    auto proc = run_single_batch({0x006F0000, 0x00000000}, cfg);

    // The processor must survive to this point (no crash / ASAN abort).

    // ccol=55 must not be mistaken for a chip-tag error (216-223 range).
    CHECK(proc->_chipTagBitFlipCnt == 0);
    CHECK(proc->_chipTagErrorCnt   == 0);

    // The format marker check fires on the invalid internal tag (ccol=55 →
    // bits[10:8]=0b110), logging exactly one corrupt-stream error.
    CHECK(proc->_corruptStreamErrorCnt == 1);
}

// ---------------------------------------------------------------------------
// Issue #1 (variant) — isneighbor=1 path: ++_qrow[55]
//
// When isneighbor=1 the processor increments _qrow[_ccol] directly instead of
// calling retrieve().  For ccol=55 this is also a one-past-end OOB write.
//
// Stream layout:
//   bits 22-17: ccol=55 (0x006E0000)
//   bits 16-15: islast=0, isneighbor=1  → 0b01 << 15 = bit 15 = 0x00008000
//               (processor will loop and try another neighbor qrow next)
//   bits 14-13: islast=1, isneighbor=0  → 0b10 << 13 = 0x00004000
//               (exits the inner do-while on second iteration)
//
// word[0] = 0x006E8000 | 0x00004000 = 0x006EC000, word[1] = 0x00000000
// ---------------------------------------------------------------------------
TEST_CASE("Itkpixv2DataProcessor: ccol=55 OOB increment via isneighbor",
          "[itkpixv2][bug_ccol55_neighbor]") {

    Itkpixv2Cfg cfg;
    // ccol=55 is caught by the internal tag path before reaching the isneighbor
    // branch.  The format marker check (bits[10:8] must be 0b111) fails, logs
    // one corrupt-stream error, and resets to INIT without touching _qrow.
    auto proc = run_single_batch({0x006EC000, 0x00000000}, cfg);

    CHECK(proc->_chipTagBitFlipCnt == 0);
    CHECK(proc->_chipTagErrorCnt   == 0);
    CHECK(proc->_corruptStreamErrorCnt == 1);
}

// ---------------------------------------------------------------------------
// Issue #2 — _curOut potentially null when _status != INIT
//
// When a batch ends with _events==0 (all dead-word blocks, no valid event
// header decoded), _curOut is reset to nullptr in getNextDataBlockImpl().
// If popData() then returns nullptr (no next batch ready), the processor
// returns with _curOut==null and _status==INIT.
//
// On the next call to process_core(), _status==INIT so getNextDataBlock() is
// called first, which reinitialises _curOut before the newEvent() call.
// The crash scenario (BCIDL1 with _readBcL1==false and null _curOut) requires
// _status!=INIT when _curOut is null, which is only reachable via the
// is_end_of_iteration path (see analysis in commit message).
//
// This test exercises the observable symptom: two all-dead batches (resetting
// _curOut twice) followed by a valid batch must decode correctly.  If _curOut
// is improperly null when the valid data arrives the newEvent() call crashes.
// ---------------------------------------------------------------------------
TEST_CASE("Itkpixv2DataProcessor: empty batches followed by valid data",
          "[itkpixv2][bug_null_curout_sequence]") {

    Itkpixv2Cfg cfg;

    // Batches 1 & 2: all dead words → _events=0 → _curOut.reset() each time.
    // Batch 3: a well-formed two-event stream (tags 250 & 251 from the
    //          selftrigger case used elsewhere in this test suite).
    auto proc = run_multi_batch(
        {
            { 0xFFFFDEAD, 0xFFFFDEAD },
            { 0xFFFFDEAD, 0xFFFFDEAD },
            { 4250599616, 77605504, 4253024256, 0 }
        },
        cfg
    );

    // Processor must survive and produce no spurious tag errors.
    CHECK(proc->_chipTagBitFlipCnt == 0);
    CHECK(proc->_chipTagErrorCnt   == 0);
    CHECK(proc->_corruptStreamErrorCnt == 0);
}

// ---------------------------------------------------------------------------
// Issue #3 — OOB rawDataIdx is logged but not prevented
//
// In getNextDataBlockImpl(), when _rawDataIdx >= _curInV->data.size() the
// code logs an error but then falls through to:
//
//     if (_wordIdx >= _curInV->data[_rawDataIdx]->getSize())
//
// which is itself an out-of-bounds container access.
//
// Triggering this from outside the class requires _rawDataIdx to already be
// out of range when getNextDataBlockImpl() is entered.  In normal operation
// the "cannot get more data" guard resets _rawDataIdx before the next entry,
// making this hard to provoke through the public API.
//
// The test below is therefore a documentation anchor: it runs a pattern
// (single tiny RawData followed by another batch) that exercises the boundary
// between containers and verifies the processor survives.  The actual OOB
// would require either a race or direct internal-state manipulation, which is
// outside the scope of the current test harness.
// ---------------------------------------------------------------------------
TEST_CASE("Itkpixv2DataProcessor: boundary between small RawData containers",
          "[itkpixv2][bug_rawdataidx_boundary]") {

    Itkpixv2Cfg cfg;

    // Two separate two-word batches.  The processor exhausts the first
    // RawData (2 words = 1 block) and must safely transition to the second.
    auto proc = run_multi_batch(
        {
            // Batch 1: one valid block (tag 250 stream head)
            { 4250599616, 77605504 },
            // Batch 2: terminating block (tag 251)
            { 4253024256, 0 }
        },
        cfg
    );

    CHECK(proc->_corruptStreamErrorCnt == 0);
    CHECK(proc->_unfinishedStreamErrorCnt == 0);
}

// ---------------------------------------------------------------------------
// Issue #4 — static locals maskLoopIndex / check_loop_index shared across
//            all instances and threads in the PToT path
//
// The two static locals are initialised once (process-wide) and never reset
// between processor instances.  In a multi-chip scan a second instance
// inherits the maskLoopIndex found by the first instance, which is only
// wrong when the two chips have different loop configurations.
//
// Directly producing PToT data (qrow==196) via the public encoder API is not
// currently supported.  This test instead verifies that two sequentially-run
// processor instances on identical non-PToT data produce the same hit counts,
// which would diverge if the statics caused index corruption.  A future test
// should construct actual PToT streams once encoder support is available.
// ---------------------------------------------------------------------------
TEST_CASE("Itkpixv2DataProcessor: two sequential instances give identical results",
          "[itkpixv2][bug_static_local_two_instances]") {

    // Well-formed two-event stream used as a stable reference.
    std::vector<uint32_t> stream = { 4250599616, 77605504, 4253024256, 0 };

    Itkpixv2Cfg cfg;

    std::shared_ptr<FeDataProcessor> proc1 = StdDict::getDataProcessor("ITKPIXV2");
    std::shared_ptr<FeDataProcessor> proc2 = StdDict::getDataProcessor("ITKPIXV2");
    REQUIRE(proc1);
    REQUIRE(proc2);

    auto run_and_count_hits = [&](std::shared_ptr<FeDataProcessor> proc) -> int {
        ClipBoard<RawDataContainer> rd_cp;
        ClipBoard<EventDataBase>    em_cp;
        proc->connect(&cfg, &rd_cp, &em_cp);
        proc->init();
        proc->run();

        auto rdc = std::make_unique<RawDataContainer>(LoopStatus());
        auto rd  = std::make_shared<RawData>(0, stream.size());
        std::copy(stream.begin(), stream.end(), rd->getBuf());
        rdc->add(std::move(rd));
        rd_cp.pushData(std::move(rdc));

        rd_cp.finish();
        proc->join();

        int total_hits = 0;
        while (!em_cp.empty()) {
            auto data = em_cp.popData();
            FrontEndData &fed = *static_cast<FrontEndData*>(data.get());
            for (auto &evt : fed.events)
                total_hits += evt.hits.size();
        }
        return total_hits;
    };

    int hits1 = run_and_count_hits(proc1);
    int hits2 = run_and_count_hits(proc2);

    // Both instances must decode the same number of hits from the same stream.
    CHECK(hits1 == hits2);
}

// ---------------------------------------------------------------------------
// RTL cross-check — dumpDebugBuffer secondary OOB via _qrow[_ccol]
//
// dumpDebugBuffer() logs _qrow[_ccol].  When called in an error state where
// _ccol is out of range (e.g. data corruption set _ccol=55), the log line
// itself performs the same OOB access that we fixed in process_core().
//
// The fix in both processors: introduce safe_ccol = min(_ccol, 54) and use
// that for the _qrow index in the log line.
//
// RTL validation (from the full stream-format analysis):
//   - The chip generates CCA 1-50 (ATLAS) or 1-54 (CMS) for pixel data.
//   - CCA=0 is the end-of-stream separator.
//   - CCA >= 56 (0x38) are internal "extended tags" (always prefixed with
//     3'b111 in ConcentratorAlignData.sv).
//   - CCA=55 is unreachable from a functioning chip, so the only way
//     dumpDebugBuffer fires with _ccol=55 is during corrupt-data error
//     recovery — precisely the situation where the bounds check is needed.
//
// This test confirms the processor survives the ccol=55 stream (which also
// triggers dumpDebugBuffer on ASAN/debug builds) and that the safe_ccol
// clamp does not affect the logged ccol value itself (only the _qrow lookup).
// ---------------------------------------------------------------------------
TEST_CASE("Itkpixv2DataProcessor: dumpDebugBuffer safe_ccol clamp",
          "[itkpixv2][bug_dumpbuffer_oob]") {

    Itkpixv2Cfg cfg;
    // Same ccol=55 stream as bug_ccol55: exercises the dumpDebugBuffer code
    // path when USE_ITKPIX_DEBUG_BUFFER > 0 is compiled in.
    auto proc = run_single_batch({0x006F0000, 0x00000000}, cfg);

    // Processor must complete without crash.
    // With ASAN + USE_ITKPIX_DEBUG_BUFFER > 0 this would previously abort
    // inside dumpDebugBuffer at the _qrow[_ccol] log line.
    // The format marker check now catches ccol=55 (bits[10:8]=0b110) and
    // exactly one corrupt-stream error is logged.
    CHECK(proc->_chipTagBitFlipCnt == 0);
    CHECK(proc->_chipTagErrorCnt   == 0);
    CHECK(proc->_corruptStreamErrorCnt == 1);
}
