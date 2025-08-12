#include "catch.hpp"

#include "LoopStatus.h"
#include "catch_amalgamated.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>

TEST_CASE("LoopStatus UID", "[Yarr][LoopStatus]") {
  // create our loop status vectors
  std::vector<unsigned> loops;
  SECTION("Single Loop") { loops = {745}; }
  SECTION("Single Loop Maximum Iterations") {
    loops = {std::numeric_limits<unsigned>::max()};
  }
  SECTION("Multiple Loops") { loops = {745, 1234, 5678}; }
  SECTION("Maximum Loops") {
    loops = {75834, 4839, 19082, 39, 0, 2901, 5893, 123456};
  }
  SECTION("Maximum Loops Maximum Iterations") {
    loops = std::vector<unsigned>(8, std::numeric_limits<unsigned>::max());
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

TEST_CASE("LoopStatus Masked UID", "[Yarr][LoopStatus]") {

  SECTION("Single Loop Mask") {
    std::vector<unsigned> loops = {0xf0f0f0f0, 0xff00ff00, 0xffff0000};
    std::vector<LoopStyle> styles(loops.size(), LOOP_STYLE_PARAMETER);
    size_t loopToMask = 1;

    LoopStatus stat(loops, styles);
    LoopStatus::UID id = stat.maskedUniqueID(loopToMask);

    LoopStatus::UID expected;
    for (size_t i = 0; i < loops.size(); i++) {
      if (i == loopToMask) {
        continue;
      }
      expected |= (static_cast<LoopStatus::UID>(loops[i])
                   << (sizeof(unsigned) * 8 * i));
    }
    REQUIRE(id == expected);
  }

  SECTION("Multiple Loop Mask") {
    std::vector<unsigned> loops = {0xf0f0f0f0, 0xff00ff00, 0xffff0000};
    std::vector<LoopStyle> styles(loops.size(), LOOP_STYLE_PARAMETER);
    std::vector<size_t> loopsToMask = {0, 2};

    LoopStatus stat(loops, styles);
    LoopStatus::UID id = stat.maskedUniqueID(loopsToMask);

    LoopStatus::UID expected;
    for (size_t i = 0; i < loops.size(); i++) {
      if (std::find(loopsToMask.begin(), loopsToMask.end(), i) !=
          loopsToMask.end()) {
        continue;
      }

      expected |= (static_cast<LoopStatus::UID>(loops[i])
                   << (sizeof(unsigned) * 8 * i));
    }
    REQUIRE(id == expected);
  }
}
