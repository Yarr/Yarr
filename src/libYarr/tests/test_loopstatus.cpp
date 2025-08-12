#include "catch.hpp"

#include "LoopStatus.h"

#include <limits>

TEST_CASE("LoopStatus UID", "[Yarr][LoopStatus]") {
  // create our loop status vectors
  std::vector<unsigned> loops;
  SECTION("Single Loop") { loops = {745}; }
  SECTION("Single Loop Maximum Iterations") {
    loops = {std::numeric_limits<unsigned>::max()};
  }
  SECTION("Multiple Loops") { std::vector<unsigned> loops = {745, 1234, 5678}; }
  SECTION("Maximum Loops") {
    std::vector<unsigned> loops = {75834, 4839, 19082, 39,    0,
                                   2901,  5893, 89328, 123456};
  }
  SECTION("Maximum Loops Maximum Iterations") {
    std::vector<unsigned> loops(8, std::numeric_limits<unsigned>::max());
  }

  // just fill in styles with LOOP_STYLE_PARAMETER
  std::vector<LoopStyle> styles(loops.size(), LOOP_STYLE_PARAMETER);

  // initialize our LoopStatus
  LoopStatus stat(loops, styles);
  LoopStatus::UID id = stat.uniqueID();

  // build an expected UID and compare
  LoopStatus::UID expected;
  for (size_t i = 0; i < loops.size(); i++) {
    expected |=
        (static_cast<LoopStatus::UID>(loops[i]) << (sizeof(unsigned) * 8 * i));
  }
  REQUIRE(id == expected);
}
