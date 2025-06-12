#include "catch.hpp"

#include <iostream>

#include "AllProcessors.h"
#include "StarCfg.h"
#include "StarProcessor.h"

#include "EventData.h"
#include <memory>

TEST_CASE("StarDataProcessor", "[star][data_processor]") {
  std::shared_ptr<FeDataProcessor> proc;

  bool use_base_zero = false;

  SECTION("Star") {
    proc = StdDict::getDataProcessor("Star");
  }

  SECTION("Star_PPA") {
    proc = StdDict::getDataProcessor("Star_vH0A1");
  }

  SECTION("Star_PPB") {
    proc = StdDict::getDataProcessor("Star_vH1A1");
  }

  SECTION("Star_PPB_template") {
    proc = StdDict::getDataProcessor("Star_vH1A1");
    json j;
    j["use_template"] = true;
    proc->loadConfig(j);
  }

  SECTION("Star_PPB_template_base0") {
    use_base_zero = true;

    proc = StdDict::getDataProcessor("Star_vH1A1");
    json j;
    j["use_template"] = true;
    j["zero_base"] = true;
    proc->loadConfig(j);
  }

  CAPTURE (use_base_zero);

  REQUIRE (proc);

  // Use v1 mapping for simplicity
  int hcc_version = 1;
  StarCfg starCfg(0, hcc_version);
  starCfg.hcc().setSubRegisterValue(HCCStarSubRegister::ICENABLE, 0x3ff);

  proc->connect(&starCfg, nullptr, nullptr);

  proc->init();

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

  auto output = proc->process_event_core(*rdc, [](auto f) {});

  // No other channels added
  REQUIRE (output);

  auto &data = output;
  FrontEndData &rawData = *(FrontEndData*)data.get();

  REQUIRE (rawData.events.size() == 1);

  auto &first = rawData.events.front();
  REQUIRE (first.l1id == 0);
  REQUIRE (first.bcid == 3);
  REQUIRE (first.nHits == expected.size());

  std::vector<uint16_t> out_hits;

  for(auto &hit: first.hits) {
    // Remove offset to base 1
    uint16_t packed = hit.col - (use_base_zero?0:1);
    size_t check_row = hit.row - (use_base_zero?0:1);
    CAPTURE(hit.row, hit.col);
    CAPTURE(check_row);
    if(check_row == 1) {
      packed |= 0x8000;
    } else {
      CHECK (check_row == 0);
    }
    out_hits.push_back(packed);

    REQUIRE (hit.tot == 1); // Required for histogramming
  }

  REQUIRE (out_hits.size() == expected.size());
  std::sort(out_hits.begin(), out_hits.end());

  CAPTURE (out_hits, expected);
  for(size_t index=0; index<out_hits.size(); index++) {
    CAPTURE (index);
    bool found_row = out_hits[index] & 0x8000;
    bool expect_row = expected[index] & 0x8000;
    uint32_t found_channel_offset = out_hits[index] & 0x7f80;
    uint32_t expect_channel_offset = expected[index] & 0x7f80;
    uint32_t found_strip = out_hits[index] & 0x7f;
    uint32_t expect_strip = expected[index] & 0x7f;

    CAPTURE (found_row, found_channel_offset, found_strip, expect_row, expect_channel_offset, expect_strip);

    CHECK (found_row == expect_row);
    CHECK (found_channel_offset == expect_channel_offset);
    CHECK (found_strip == expect_strip);
  }
}

TEST_CASE("StarDataProcessorPrintTemplate", "[star][data_processor]") {
  std::vector<uint8_t> bytes;
  std::string expected;

  // In each section test particular template function
  SECTION("Nothing") {
    expected = "Parse error: Not enough words\n";
  }

  SECTION("Error Info") {
    std::string header = "Packet type is TYP_LP\nPacket info: BCID 3 (0), L0ID 0\n";

    std::string error_marker = "We received an error block 0x77F4!\nReceived packet errors for the following channels:\n";

    std::string abc_map = "  ABC Error: 4\n  BCID Error: 2 9\n  L0tag Error: 0 7\n  Timeout Error: 5\n";

    expected = header + error_marker + abc_map;

    bytes = {
      0x20, 0x06, // Header
      0x77, 0xf4, // Error marker
      0x01, 0x02, // 12 bits for each of 4 types
      0x04, 0x08,
      0x10, 0x20,
      0x6f, 0xed, // Trailer
    };
  }

  SECTION("Data Header") {
    expected = "Packet info: BCID 3 (0), L0ID 0\n";

    expected = "Packet type is TYP_LP\n" + expected;

    bytes = {
      0x20, 0x06, // Header
      0x6f, 0xed,  // Trailer
    };
  }

  SECTION("Data No cluster") {
    expected = "Packet's abc clusters are:\n  -) Empty chip (2)\n";

    // Surrounding
    expected = "Packet type is TYP_LP\nPacket info: BCID 3 (0), L0ID 0\n" + expected;

    bytes = {
      0x20, 0x06, // Header
      0x13, 0xfe,
      0x6f, 0xed  // Trailer
    };
  }

  SECTION("Data Cluster") {
    expected = "Packet's abc clusters are:\n  0) InputChannel: 9, Address: 0xe5, Next Strip Pattern: 000.\n";

    expected = "Packet type is TYP_LP\nPacket info: BCID 3 (0), L0ID 0\n" + expected;

    bytes = {
      0x20, 0x06, // Header
      0x4f, 0x28, // Hit input channel 9, 0xe5 only
      0x6f, 0xed  // Trailer
    };
  }

  SECTION("ABC Read") {
    expected = " ABC 0 ABC status 9141 Address: 51 value: 0000000c\n";

    expected = "Packet type is TYP_ABC_RR\n" + expected;

    bytes = {
      0x40, 0x33,
      0x00, 0x00,
      0x00, 0x00,
      0xc9, 0x14,
      0x10
    };
  }

  SECTION("ABC HPR Read") {
    expected = " ABC 0 ABC status 9149 Address: 63 value: 78551fff\n";

    expected = "Packet type is TYP_ABC_HPR\n" + expected;

    bytes = {
      0xd0, 0x3f,
      0x07, 0x85,
      0x51, 0xff,
      0xf9, 0x14,
      0x90,
    };
  }

  SECTION("HCC Read") {
    expected = " Address: 3 value: 74b78557\n";

    expected = "Packet type is TYP_HCC_RR\n" + expected;

    bytes = {
      0x80, 0x37,
      0x4b, 0x78,
      0x55, 0x70,
    };
  }

  SECTION("HCC HPR Read") {
    expected = " Address: 15 value: 57850079\n";

    expected = "Packet type is TYP_HCC_HPR\n" + expected;

    bytes = {
      0xe0, 0xf5,
      0x78, 0x50,
      0x07, 0x90,
    };
  }

  SECTION("Transparent") {
    expected = "ABC transparent 0: 0010001000000101101111111110011111111111011111111111011111111111\n";

    expected = "Packet type is TYP_ABC_TRANSP\n" + expected;

    bytes = {
      0x70, 0x22,
      0x05, 0xbf,
      0xe7, 0xff,
      0x7f, 0xf7,
      0xff,
    };
  }

  CAPTURE (bytes);

  std::stringstream oss;
  PrintProc proc(oss);

  StarProcessPacket(bytes.data(), bytes.data() + bytes.size(), proc);
  std::string output = oss.str();
  CHECK (output == expected);
  CHECK (output.size() == expected.size());
  for (int i=0; i<output.size(); i++) {
    CAPTURE(i);
    if(i) {
      CAPTURE(output[i-1]);
    }
    if(i<output.size()-1) {
      CAPTURE(output[i+1]);
    }
    CHECK(output[i] == expected[i]);
  }
}

namespace {

struct TC {
  std::vector<uint32_t> input;
  std::vector<std::pair<uint16_t, uint16_t>> output;
};

}

TEST_CASE("StarDataProcessorRaw", "[star][data_processor]") {
  // Use default, no difference for ASIC versions
  std::shared_ptr<FeDataProcessor> proc
    = StdDict::getDataProcessor("Star");

  REQUIRE (proc);

  json j;
  j["raw_bits"] = true;
  proc->loadConfig(j);

  proc->init();

  auto tc = GENERATE
    (
     TC{{0x87654321}, {{0x8765, 0x4321}}},
     TC{{0x1, 2, 3, 4}, {{0, 0x1}, {0, 0x2}, {0, 0x3}, {0, 0x4}}}
    );

  size_t len = tc.input.size();

  RawDataPtr rd = std::make_shared<RawData>(0, len);
  uint32_t *buffer = rd->getBuf();
  std::copy(tc.input.data(), tc.input.data()+len, buffer);

  std::unique_ptr<RawDataContainer> rdc(new RawDataContainer(LoopStatus()));
  rdc->add(std::move(rd));

  auto output = proc->process_event_core(*rdc, [](auto f) {});

  // No other channels added
  REQUIRE (output);

  auto data = dynamic_cast<FrontEndData*>(output.get());
  REQUIRE (data);

  FrontEndData &rawData = *data;

  REQUIRE (rawData.events.size() == 1);

  auto &event = rawData.events.front();
  CHECK (event.l1id == 0xffff);
  CHECK (event.bcid == 0xffff);

  std::vector<uint16_t> out_hits;

  CHECK ( event.hits.size() == tc.output.size() );

  for(size_t o=0; o<event.hits.size(); o++) {
    auto &hit = event.hits[o];
    CAPTURE(hit.row, hit.col);

    auto &exp = tc.output[o];

    CHECK (hit.row == exp.first);
    CHECK (hit.col == exp.second);

    // Marker
    CHECK (hit.tot == 0xffff);
  }
}
