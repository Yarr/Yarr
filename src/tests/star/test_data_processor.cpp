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
    0x20, 0x06,
    0x03, 0x8f,
    0x0b, 0xaf,
    0x6f, 0xed
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

  REQUIRE (!em_cp[chan].empty());

  auto data = em_cp[chan].popData();
  std::cout << "Data: " << &data << "\n";
  ((Fei4Data*)data.get())->toFile("/dev/stdout");

  // Only one thing
  REQUIRE (em_cp[chan].empty());
}
