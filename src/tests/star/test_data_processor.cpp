#include "catch.hpp"

#include <iostream>

#include "AllProcessors.h"

#include "Fei4EventData.h"

TEST_CASE("StarDataProcessor", "[star][data_processor]") {
  std::shared_ptr<DataProcessor> proc = StdDict::getDataProcessor("Star");

  REQUIRE (proc);

  ClipBoard<RawDataContainer> rd_cp;
  std::map<unsigned, ClipBoard<EventDataBase> > em_cp;

  int chan = 0;
  em_cp[chan];

  proc->connect( &rd_cp, &em_cp );

  proc->init();
  proc->run();

  alignas(32) uint8_t packet_bytes[] = {
    0x20, 0x06, // Header
    0x03, 0x8f, // Hit input channel 0, 0x71 and three following
    0x0b, 0xaf, // Hit input channel 1, 0x75 and three following
    0x4f, 0x28, // Hit input channel 9, 0xe5 only
    0x5c, 0x05, // Hit input channel 11, 0x80 and 101
    0x1b, 0xf4, // Hit input channel 3, 0x7e and 1 following
    0x6f, 0xed  // Trailer
  };

  // Pack row (top bit) and column in one so we can sort them
  std::vector<uint16_t> expected = {
    0x0071, 0x0072, 0x0073, 0x0074,
    0x00f5, 0x00f6, 0x00f7, 0x00f8,
    0x84e5,
    0x8580, 0x8581, 0x8583,
    0x01fe, 0x01ff
  };

  std::sort(expected.begin(), expected.end());

  size_t len_bytes = sizeof(packet_bytes);
  size_t len = (len_bytes+3)/sizeof(uint32_t);

  uint32_t *buffer = new uint32_t[len];
  buffer[len-1] = 0;
  // Could copy uint32, but then the extra bytes are undefined 
  std::copy(packet_bytes, packet_bytes+len_bytes, (uint8_t*)buffer);

  RawData *rd = new RawData(chan, buffer, len);
  std::unique_ptr<RawDataContainer> rdc(new RawDataContainer(LoopStatus::empty()));
  rdc->add(rd);
  rd_cp.pushData(std::move(rdc));

  rd_cp.finish();

  proc->join();

  // No other channels added
  REQUIRE (em_cp.size() == 1);
  REQUIRE (!em_cp[chan].empty());

  auto data = em_cp[chan].popData();
  Fei4Data &rawData = *(Fei4Data*)data.get();

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

  for(size_t o=0; o<out_hits.size(); o++) {
    CAPTURE (o);
    bool found_row = out_hits[o] & 0x8000;
    bool expect_row = expected[o] & 0x8000;
    uint32_t found_channel_offset = out_hits[o] & 0x7f80;
    uint32_t expect_channel_offset = expected[o] & 0x7f80;
    uint32_t found_strip = out_hits[o] & 0x7f;
    uint32_t expect_strip = expected[o] & 0x7f;

    CAPTURE (found_row, found_channel_offset, found_strip, expect_row, expect_channel_offset, expect_strip);

    REQUIRE (found_row == expect_row);
    REQUIRE (found_channel_offset == expect_channel_offset);
    REQUIRE (found_strip == expect_strip);
  }

  // Only one thing
  REQUIRE (em_cp[chan].empty());
}
