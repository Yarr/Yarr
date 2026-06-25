#include "catch.hpp"

#include "AsyncReg.h"

#include "EmptyHw.h"
#include "EmptyFrontEnd.h"
#include "logging.h"

namespace {
auto logger = logging::make_log("test_async");
}

TEST_CASE("AsyncReadRegister", "[Async]") {
  auto l = spdlog::get("StarChips");

  EmptyHw hw;

  logger->debug("++ Make async");
  AsyncAccess::AsyncContext anon{hw, hw};

  logger->debug("++ Do async read");
  AsyncAccess::AsyncReg something(anon,
                     [](TxCore &){},
                     [](const RawData &){ return true; },
                     [](const RawData &){});

  logger->debug("++ Async read in progress");

  something.result.get();
  logger->debug("++ Finish async test");
}
