#include "catch.hpp"

#include "LCBUtils.h"

TEST_CASE("6b8b check", "[star][lcb]") {
  std::vector<bool> v(256);

  for(int i=0; i<64; i++) {
    uint8_t encoded = SixEight::encode(i);
    CAPTURE ( i, (int)encoded );
    REQUIRE ( SixEight::decode(encoded) == i );
    REQUIRE ( SixEight::is_valid(encoded) );
    v[encoded] = true;
  }

  for(int k=0; k<4; k++) {
    uint8_t encoded = SixEight::kcode(k);
    REQUIRE ( SixEight::is_valid(SixEight::kcode(k)) );
    v[encoded] = true;
  }

  for(int i=0; i<256; i++) {
    REQUIRE ( SixEight::is_valid(i) == v[i] );
  }
}

TEST_CASE("Fast Command", "[star][lcb]") {
  for(int d=0; d<4; d++) {
    for(int c=0; c<16; c++) {
      LCB::Frame f = LCB::fast_command((LCB::FastCmdType)c, d);
      
      CAPTURE (d, c,
               LCB::is_l0a_bcr(f),
               LCB::is_fast_command(f),
               LCB::get_fast_command_bc(f) );
      REQUIRE ( LCB::is_fast_command(f) );
      REQUIRE ( !LCB::is_l0a_bcr(f) );
      REQUIRE ( LCB::get_fast_command(f) == c);
      REQUIRE ( LCB::get_fast_command_bc(f) == d);
    }
  }
}

TEST_CASE("L0A Test", "[star][lcb]") {
  for(int m=0; m<16; m++) {
    for(int t=0; t<128; t++) {
      for(int b=0; b<2; b++) {
        if ( m == 0 && b == 0) {
          continue;
        }

        LCB::Frame f = LCB::l0a_mask(m, t, b);
      
        CAPTURE (m, t, b,
                 LCB::is_l0a_bcr(f),
                 LCB::is_fast_command(f) );

        uint8_t fst, sec;
        std::tie(fst, sec) = LCB::split_pair(f);
        CAPTURE ( (int)fst, (int)sec );
        CAPTURE ( (int)SixEight::decode(fst), (int)SixEight::decode(sec) );

        REQUIRE ( LCB::is_l0a_bcr(f) );
        REQUIRE ( !LCB::is_fast_command(f) );
        REQUIRE ( LCB::get_l0_mask(f) == m);
        REQUIRE ( LCB::get_l0_tag(f) == t);
      }
    }
  }
}

TEST_CASE("Other data", "[star][lcb]") {
  for(int d=0; d<128; d++) {
    LCB::Frame f = LCB::command_bits(d);
      
    CAPTURE ( d,
              LCB::is_l0a_bcr(f),
              LCB::is_fast_command(f) );
    REQUIRE ( !LCB::is_l0a_bcr(f) );
    REQUIRE ( !LCB::is_fast_command(f) );
  }
}
