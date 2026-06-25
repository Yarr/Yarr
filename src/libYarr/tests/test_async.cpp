#include "catch.hpp"

#include "AsyncReg.h"

#include "EmptyHw.h"
#include "EmptyFrontEnd.h"
#include "logging.h"

TEST_CASE("AsyncReadRegister", "[Async]") {
  auto l = spdlog::get("StarChips");

  EmptyHw hw;

  AsyncAccess::AsyncContext anon{hw, hw};

  AsyncAccess::AsyncReg something(anon,
                     [](TxCore &){},
                     [](const RawData &){ return true; },
                     [](const RawData &){});

  something.result.get();
}
