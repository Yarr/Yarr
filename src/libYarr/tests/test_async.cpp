#include "catch.hpp"

#include "AsyncReg.h"

#include "EmptyHw.h"
#include "EmptyFrontEnd.h"
#include "logging.h"

namespace {
auto logger = logging::make_log("test_async");

class MyTestHw: public EmptyHw {
public:
  std::vector<std::vector<RawDataPtr>> data;

  std::vector<RawDataPtr> readData() override {
    if(data.empty()) {
      return {};
    }

    auto elem = data.back();

    logger->debug("ReadData {}", elem.size());

    std::vector<RawDataPtr> result{elem};
    data.pop_back();

    return result;
  }
}; // End class

} // end namespace

TEST_CASE("AsyncReadRegister", "[Async]") {
  auto l = spdlog::get("StarChips");

  MyTestHw hw;

  logger->debug("++ Make async");
  AsyncAccess::AsyncContext anon{hw, hw};

  logger->debug("++ Do async read");
  uint32_t rx_channel = 0;
  AsyncAccess::AsyncReg something(anon,
                     [&](TxCore &){
                       hw.data.push_back({std::make_unique<RawData>(rx_channel, std::vector<uint32_t>{})});
                     },
                     [](const RawData &){ return true; },
                     [](const RawData &){});

  logger->debug("++ Async read in progress");

  something.result.get();
  logger->debug("++ Finish async test");
}
