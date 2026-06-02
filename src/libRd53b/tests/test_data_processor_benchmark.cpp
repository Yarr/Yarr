/*
 * Throughput benchmarks for Rd53bDataProcessor.
 *
 * These tests are excluded from normal test runs (Catch2 [!benchmark] tag).
 * Run them explicitly:
 *
 *   bin/testRd53b '[!benchmark]'
 *   bin/testRd53b '[rd53b][!benchmark]'
 *   bin/testRd53b '[!benchmark]' --benchmark-no-analysis
 *
 * See the Itkpixv2 counterpart for full commentary on methodology.
 * The only differences here are the encoder class (Rd53bEncoder) and chip
 * config (Rd53bCfg); occupancy scenarios and event counts are identical.
 */

#include "catch.hpp"

#include <vector>

#include "AllProcessors.h"
#include "ClipBoard.h"
#include "EventData.h"
#include "HitMapGenerator.h"
#include "Rd53bCfg.h"
#include "Rd53bEncoder.h"
#include "RawData.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::vector<uint32_t>
make_rd53b_stream(float occupancy, int n_events, int n_per_stream = 16,
                  unsigned seed = 42)
{
    HitMapGenerator gen;
    gen.setSeed(seed);
    Rd53bEncoder encoder;
    encoder.setEventsPerStream(n_per_stream);
    for (int i = 0; i < n_events; i++) {
        gen.randomHitMap(occupancy);
        encoder.addToStream(gen.outHits(), i == n_events - 1);
    }
    return encoder.getWords();
}

static std::vector<uint32_t>
make_rd53b_scan_stream(int n_batches, unsigned seed = 42)
{
    constexpr int   EVENTS_PER_BURST = 17;
    constexpr float HIT_OCC          = 320.0f / (400.0f * 384.0f);

    HitMapGenerator gen;
    gen.setSeed(seed);
    Rd53bEncoder encoder;
    encoder.setEventsPerStream(EVENTS_PER_BURST);

    const int total = n_batches * EVENTS_PER_BURST;
    for (int i = 0; i < total; i++) {
        bool is_hit_event = ((i % EVENTS_PER_BURST) == EVENTS_PER_BURST - 1);
        gen.randomHitMap(is_hit_event ? HIT_OCC : 0.0f);
        encoder.addToStream(gen.outHits(), i == total - 1);
    }
    return encoder.getWords();
}

static size_t run_rd53b_batch(const std::vector<uint32_t>& words)
{
    Rd53bCfg cfg;
    ClipBoard<RawDataContainer> rd_cp;
    ClipBoard<EventDataBase>    em_cp;

    auto proc = StdDict::getDataProcessor("RD53B");
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

TEST_CASE("Rd53bDataProcessor throughput",
          "[rd53b][!benchmark]")
{
    constexpr int N_LOW  = 500;
    constexpr int N_MED  = 100;
    constexpr int N_HIGH = 10;
    constexpr int N_SCAN = 100;

    const auto words_low  = make_rd53b_stream(1e-4f, N_LOW);
    const auto words_med  = make_rd53b_stream(1e-2f, N_MED);
    const auto words_high = make_rd53b_stream(0.1f,  N_HIGH);
    const auto words_scan = make_rd53b_scan_stream(N_SCAN);

    BENCHMARK("low  occ (1e-4, 500 events)") {
        return run_rd53b_batch(words_low);
    };

    BENCHMARK("med  occ (1e-2, 100 events)") {
        return run_rd53b_batch(words_med);
    };

    BENCHMARK("high occ (0.1,  10 events)") {
        return run_rd53b_batch(words_high);
    };

    BENCHMARK("scan pattern (1-in-17, 100 bursts, ~320 hits/burst)") {
        return run_rd53b_batch(words_scan);
    };
}
