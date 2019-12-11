#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "StarChipPacket.h"

TEST_CASE("StarChipParser", "[star][parser]") {
  StarChipPacket p(false);

  const uint8_t hpr_bytes[] = {
    0xe0, 0xf5, 0x78, 0x50, 0x07, 0x90
  };

  p.add_word(0x13c);
  for(auto b: hpr_bytes) {
    p.add_word(b);
  }
  p.add_word(0x1dc);
  REQUIRE (p.parse() == 0);
}
