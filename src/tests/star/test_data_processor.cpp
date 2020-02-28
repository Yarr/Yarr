#include "catch.hpp"

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
    0x6f, 0xed  // Trailer
  };

  size_t len = sizeof(packet_bytes)/sizeof(uint32_t);

  uint32_t *buffer = new uint32_t[len];
  std::copy((uint32_t*)packet_bytes, ((uint32_t*)packet_bytes)+len, buffer);

  RawData *rd = new RawData(chan, buffer, len);
  std::unique_ptr<RawDataContainer> rdc(new RawDataContainer);
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
  REQUIRE (first.nHits == 8);

  std::vector<int> col_hits;

  for(auto &hit: first.hits) {
    col_hits.push_back(hit.col);

    REQUIRE (hit.row == 1);
    REQUIRE (hit.tot == 1); // Required for histogramming
  }

  REQUIRE (col_hits.size() == 8);
  std::sort(col_hits.begin(), col_hits.end());

  for(int i=0; i<8; i++) {
    int column = i + 113;
    if(i >= 4) {
      // Second cluster is input channel 1
      column += 128;
    }
    CAPTURE (i, column);
    // NB Offset to base 1
    REQUIRE (col_hits[i] == column+1);
  }

  // Only one thing
  REQUIRE (em_cp[chan].empty());
}
