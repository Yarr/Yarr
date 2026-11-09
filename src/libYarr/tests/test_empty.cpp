#include "catch.hpp"

#include "AllChips.h"
#include "AllProcessors.h"
#include "AllHwControllers.h"
#include "EmptyFrontEnd.h"
#include "EmptyDataProcessor.h"
#include "EmptyHw.h"

#include "logging.h"

namespace {
auto logger = logging::make_log("test_scan_console");

// Not registered normally, this is used elsewhere in tests
bool empty_hw = StdDict::registerHwController("TestEmpty",
                                              [](){return std::make_unique<EmptyHw>();});
bool empty_fe = StdDict::registerFrontEnd("TestEmpty",
                                          [](){return std::make_unique<EmptyFrontEnd>();});
bool empty_data_proc = StdDict::registerDataProcessor("TestEmpty",
                                          [](){return std::make_unique<EmptyDataProcessor>();});
}

// Run trivial check on EmptyHw
TEST_CASE("SimpleEmptyHw", "[EmptyHw]") {
  REQUIRE(StdDict::getHwController("TestEmpty"));
}

// Run trivial check on EmptyFrontEnd
TEST_CASE("SimpleEmptyFrontEnd", "[EmptyFrontEnd]") {
  REQUIRE(StdDict::getFrontEnd("TestEmpty"));
}

// Run trivial check on EmptyDataProcessor
TEST_CASE("SimpleEmptyDataProcessor", "[FeDataProcessor]") {
  REQUIRE(StdDict::getDataProcessor("TestEmpty"));
}
