#include "catch.hpp"

#include "BitOps.h"

TEST_CASE("BitOps", "[BitOps]") {
  for(int i=0; i<8; i++) {
    REQUIRE (BitOps::reverse_bits((uint8_t)(1<<i)) == (uint8_t)(1<<(7-i)));
  }
  REQUIRE (BitOps::reverse_bits((uint8_t)0x80) == (uint8_t)0x01);
  REQUIRE (BitOps::reverse_bits((uint8_t)0xa5) == (uint8_t)0xa5);
  REQUIRE (BitOps::reverse_bits((uint8_t)0x82) == (uint8_t)0x41);
  for(int i=0; i<16; i++) {
    REQUIRE (BitOps::reverse_bits((uint16_t)(1<<i)) == (uint16_t)(1<<(15-i)));
  }
  REQUIRE (BitOps::reverse_bits((uint16_t)0x82) == 0x4100);
}
