#include "catch.hpp"

#include <iostream>

#include "AllProcessors.h"
#include "StarCfg.h"

#include "EventData.h"
#include <memory>

TEST_CASE("StarDataProcessor", "[star][data_processor]") {
  std::shared_ptr<FeDataProcessor> proc;

  SECTION("Star") {
    proc = StdDict::getDataProcessor("Star");
  }

  SECTION("Star_PPA") {
    proc = StdDict::getDataProcessor("Star_vH0A1");
  }

  SECTION("Star_PPB") {
    proc = StdDict::getDataProcessor("Star_vH1A1");
  }

  REQUIRE (proc);

  ClipBoard<RawDataContainer> rd_cp;
  ClipBoard<EventDataBase> em_cp;

  // Use v1 mapping for simplicity
  int hcc_version = 1;
  StarCfg starCfg(0, hcc_version);
  starCfg.hcc().setSubRegisterValue("ICENABLE", 0x3ff);

  proc->connect(&starCfg, &rd_cp, &em_cp );

  proc->init();
  proc->run();

  alignas(32) uint8_t packet_bytes[] = {
    0x20, 0x06, // Header
    0x03, 0x8f, // Hit input channel 0, 0x71 and three following
    0x0b, 0xaf, // Hit input channel 1, 0x75 and three following
    0x4f, 0x28, // Hit input channel 9, 0xe5 only
    // Unphysical, check it's skipped in output
    0x5c, 0x05, // Hit input channel 11, 0x80 and 101
    // Not mapped, check it's skipped in output
    0x53, 0x00, // Hit input channel 10
    0x1b, 0xf4, // Hit input channel 3, 0x7e and 1 following
    0x6f, 0xed  // Trailer
  };

  // Pack row (top bit) and column in one so we can sort them
  std::vector<uint16_t> expected = {
    0x0071, 0x0072, 0x0073, 0x0074,
    0x00f5, 0x00f6, 0x00f7, 0x00f8,
    0x84e5,
    0x01fe, 0x01ff
  };

  std::sort(expected.begin(), expected.end());

  size_t len_bytes = sizeof(packet_bytes);
  size_t len = (len_bytes+3)/sizeof(uint32_t);

  RawDataPtr rd = std::make_shared<RawData>(0, len);
  uint32_t *buffer = rd->getBuf();
  buffer[len-1] = 0;
  // Could copy uint32, but then the extra bytes are undefined 
  std::copy(packet_bytes, packet_bytes+len_bytes, (uint8_t*)buffer);

  std::unique_ptr<RawDataContainer> rdc(new RawDataContainer(LoopStatus()));
  rdc->add(std::move(rd));
  rd_cp.pushData(std::move(rdc));

  rd_cp.finish();

  proc->join();

  // No other channels added
  REQUIRE (!em_cp.empty());

  auto data = em_cp.popData();
  FrontEndData &rawData = *(FrontEndData*)data.get();

  REQUIRE (rawData.events.size() == 1);

  auto &first = rawData.events.front();
  REQUIRE (first.l1id == 0);
  REQUIRE (first.bcid == 3);
  REQUIRE (first.nHits == expected.size());

  std::vector<uint16_t> out_hits;

  for(auto &hit: first.hits) {
    // Remove offset to base 1
    uint16_t packed = hit.col - 1;
    if((hit.row-1) == 1) {
      packed |= 0x8000;
    }
    out_hits.push_back(packed);

    REQUIRE (hit.tot == 1); // Required for histogramming
  }

  REQUIRE (out_hits.size() == expected.size());
  std::sort(out_hits.begin(), out_hits.end());

  for(size_t index=0; index<out_hits.size(); index++) {
    CAPTURE (index);
    bool found_row = out_hits[index] & 0x8000;
    bool expect_row = expected[index] & 0x8000;
    uint32_t found_channel_offset = out_hits[index] & 0x7f80;
    uint32_t expect_channel_offset = expected[index] & 0x7f80;
    uint32_t found_strip = out_hits[index] & 0x7f;
    uint32_t expect_strip = expected[index] & 0x7f;

    CAPTURE (found_row, found_channel_offset, found_strip, expect_row, expect_channel_offset, expect_strip);

    REQUIRE (found_row == expect_row);
    REQUIRE (found_channel_offset == expect_channel_offset);
    REQUIRE (found_strip == expect_strip);
  }

  // Only one thing
  REQUIRE (em_cp.empty());
}
