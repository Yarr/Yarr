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
  LoopStatus::UID expected{};
  for (size_t i = 0; i < loops.size(); i++) {
    expected[i] = loops[i];
  }
  REQUIRE(id == expected);
}

TEST_CASE("LoopStatus Masked UID", "[Yarr][LoopStatus]") {

  SECTION("Single Loop Mask") {
    std::vector<unsigned> loops = {984239, 8392, 2};
    std::vector<LoopStyle> styles(loops.size(), LOOP_STYLE_PARAMETER);
    size_t loopToMask = 1;

    LoopStatus stat(loops, styles);
    stat.setDisabledLoops({loopToMask});
    LoopStatus::UID id = stat.uniqueID();

    LoopStatus::UID expected{};
    for (size_t i = 0; i < loops.size(); i++) {
      if (i == loopToMask) {
        continue;
      }
      expected[i] = loops[i];
    }
    REQUIRE(id == expected);
  }

  SECTION("Multiple Loop Mask") {
    std::vector<unsigned> loops = {293, 90530, 38921};
    std::vector<LoopStyle> styles(loops.size(), LOOP_STYLE_PARAMETER);
    std::vector<size_t> loopsToMask = {0, 2};

    LoopStatus stat(loops, styles);
    stat.setDisabledLoops(loopsToMask);
    LoopStatus::UID id = stat.uniqueID();

    LoopStatus::UID expected{};
    for (size_t i = 0; i < loops.size(); i++) {
      if (std::find(loopsToMask.begin(), loopsToMask.end(), i) !=
          loopsToMask.end()) {
        continue;
      }

      expected[i] = loops[i];
    }
    REQUIRE(id == expected);
  }

  SECTION("Multiple Loop Enabled") {
    std::vector<unsigned> loops = {293, 90530, 38921, 1234, 5678};
    std::vector<LoopStyle> styles(loops.size(), LOOP_STYLE_PARAMETER);
    std::vector<size_t> loopsToEnable = {1, 3};

    LoopStatus stat(loops, styles);
    stat.setEnabledLoops(loopsToEnable);
    LoopStatus::UID id = stat.uniqueID();

    LoopStatus::UID expected{};
    for (size_t i = 0; i < loops.size(); i++) {
      if (std::find(loopsToEnable.begin(), loopsToEnable.end(), i) ==
          loopsToEnable.end()) {
        continue;
      }
      expected[i] = loops[i];
    }
    REQUIRE(id == expected);
  }
}

TEST_CASE("LoopStatus String", "[Yarr][LoopStatus]") {
  SECTION("Basic String") {
    std::vector<unsigned> loops = {1, 2, 3};
    std::vector<LoopStyle> styles = {LOOP_STYLE_PARAMETER, LOOP_STYLE_DATA,
                                    LOOP_STYLE_TRIGGER};
    LoopStatus stat(loops, styles);
    REQUIRE(stat.toString() == "1-2-3");
  }

  SECTION("Masked String") {
    std::vector<unsigned> loops = {1, 2, 3, 4, 5};
    std::vector<LoopStyle> styles(loops.size(), LOOP_STYLE_PARAMETER);
    LoopStatus stat(loops, styles);
    stat.setDisabledLoops({1, 3});
    REQUIRE(stat.toString() == "1-3-5");
  }
}
