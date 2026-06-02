/*
 * Throughput benchmarks for Itkpixv2DataProcessor.
 *
 * These tests are excluded from normal test runs (Catch2 [!benchmark] tag).
 * Run them explicitly:
 *
 *   bin/testItkpixv2 '[!benchmark]'                 # run all benchmarks
 *   bin/testItkpixv2 '[itkpixv2][!benchmark]'       # same, chip-scoped tag
 *   bin/testItkpixv2 '[!benchmark]' --benchmark-no-analysis  # raw samples
 *
 * Interpreting results
 * --------------------
 * Catch2 reports nanoseconds per call to the benchmark lambda.  Each lambda
 * call decodes one full batch (N_LOW / N_MED / N_HIGH events).  To get a
 * per-event or per-hit figure, divide the reported mean by the event count
 * printed in the benchmark name.
 *
 * Thread-creation overhead (~10–50 µs per run) is included in the timing and
 * is constant across all variants; it cancels in before/after comparisons.
 *
 * Occupancy scenarios
 * -------------------
 *  Low  (1e-4): ~15 hits/event – exercises stream-parsing and event-header
 *               overhead; sensitive to events.reserve() and sendFeedback()
 *               improvements.
 *  Med  (1e-2): ~1 500 hits/event – representative threshold-scan condition;
 *               sensitive to hits.reserve() and events.reserve().
 *  High (0.1) : ~15 000 hits/event – digital-scan-like; sensitive to
 *               hits.reserve() and _qrow cache density.
 *  Scan pattern (1-in-17): models the dominant real-world case where a scan
 *               streams 17 events per trigger burst, 16 of which are empty
 *               (below threshold) and 1 carries ~320 hits (64 mask groups ×
 *               5 pixels).  Sensitive to per-event allocation overhead for
 *               empty events and to hits.reserve() sizing.
 *
 * Event counts are chosen so that each scenario takes roughly 1–5 ms per
 * benchmark iteration, giving Catch2 ≥ 20 stable samples.
 */

#include "catch.hpp"

#include <vector>
#include <string>

#include "AllProcessors.h"
#include "ClipBoard.h"
#include "EventData.h"
#include "HitMapGenerator.h"
#include "Itkpixv2Cfg.h"
#include "Itkpixv2Encoder.h"
#include "RawData.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::vector<uint32_t>
make_itkpixv2_stream(float occupancy, int n_events, int n_per_stream = 16,
                     unsigned seed = 42)
{
    HitMapGenerator gen;
    gen.setSeed(seed);
    Itkpixv2Encoder encoder;
    encoder.setEventsPerStream(n_per_stream);
    for (int i = 0; i < n_events; i++) {
        gen.randomHitMap(occupancy);
        encoder.addToStream(gen.outHits(), i == n_events - 1);
    }
    return encoder.getWords();
}

// Models the dominant real-world scan pattern: N_BATCHES bursts of 17 events
// each, where 16 events are empty (below threshold) and 1 event carries
// ~320 hits (64 * 5, matching a typical mask-loop step on the full chip).
//   320 hits / 153 600 pixels ≈ 2.08e-3 per-pixel occupancy
static std::vector<uint32_t>
make_itkpixv2_scan_stream(int n_batches, unsigned seed = 42)
{
    constexpr int   EVENTS_PER_BURST = 17;
    constexpr float HIT_OCC          = 320.0f / (400.0f * 384.0f); // ~2.08e-3

    HitMapGenerator gen;
    gen.setSeed(seed);
    Itkpixv2Encoder encoder;
    encoder.setEventsPerStream(EVENTS_PER_BURST);

    const int total = n_batches * EVENTS_PER_BURST;
    for (int i = 0; i < total; i++) {
        bool is_hit_event = ((i % EVENTS_PER_BURST) == EVENTS_PER_BURST - 1);
        gen.randomHitMap(is_hit_event ? HIT_OCC : 0.0f);
        encoder.addToStream(gen.outHits(), i == total - 1);
    }
    return encoder.getWords();
}

// Run the full decode pipeline for one pre-encoded batch.
// Returns the total decoded hit count so the compiler cannot discard the work.
static size_t run_itkpixv2_batch(const std::vector<uint32_t>& words)
{
    Itkpixv2Cfg cfg;
    ClipBoard<RawDataContainer> rd_cp;
    ClipBoard<EventDataBase>    em_cp;

    auto proc = StdDict::getDataProcessor("ITKPIXV2");
    proc->connect(&cfg, &rd_cp, &em_cp);
    proc->init();
    proc->run();

    auto rdc = std::make_unique<RawDataContainer>(LoopStatus());
    auto rd  = std::make_shared<RawData>(0, static_cast<int>(words.size()));
    std::copy(words.begin(), words.end(), rd->getBuf());
    rdc->add(std::move(rd));
    rd_cp.pushData(std::move(rdc));
    rd_cp.finish();
    proc->join();

    size_t hits = 0;
    while (!em_cp.empty()) {
        auto d = em_cp.popData();
        if (auto* fe = dynamic_cast<FrontEndData*>(d.get()))
            for (const auto& evt : fe->events)
                hits += evt.nHits;
    }
    return hits;
}

// ---------------------------------------------------------------------------
// Benchmarks
// ---------------------------------------------------------------------------

TEST_CASE("Itkpixv2DataProcessor throughput",
          "[itkpixv2][!benchmark]")
{
    // Event counts tuned so each scenario takes ~1-5 ms per benchmark
    // iteration at expected decode speeds (≥ 20 Catch2 samples).
    //
    //  Low  occ: 500 events × ~15 hits   ≈     7 500 hits/iter
    //  Med  occ: 100 events × ~1 500 hits ≈   150 000 hits/iter
    //  High occ:  10 events × ~15 000 hits ≈  150 000 hits/iter

    constexpr int N_LOW   = 500;
    constexpr int N_MED   = 100;
    constexpr int N_HIGH  = 10;
    constexpr int N_SCAN  = 100; // batches of 17: 1 700 events, ~32 000 hits

    // Build streams once before any timing starts.
    const auto words_low  = make_itkpixv2_stream(1e-4f, N_LOW);
    const auto words_med  = make_itkpixv2_stream(1e-2f, N_MED);
    const auto words_high = make_itkpixv2_stream(0.1f,  N_HIGH);
    const auto words_scan = make_itkpixv2_scan_stream(N_SCAN);

    BENCHMARK("low  occ (1e-4, 500 events)") {
        return run_itkpixv2_batch(words_low);
    };

    BENCHMARK("med  occ (1e-2, 100 events)") {
        return run_itkpixv2_batch(words_med);
    };

    BENCHMARK("high occ (0.1,  10 events)") {
        return run_itkpixv2_batch(words_high);
    };

    BENCHMARK("scan pattern (1-in-17, 100 bursts, ~320 hits/burst)") {
        return run_itkpixv2_batch(words_scan);
    };
}
