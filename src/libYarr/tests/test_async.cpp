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

TEST_CASE("AsyncReadRegisterWithFilter", "[Async]") {
  // For instance reads from multiple registers discriminate by address
  auto l = spdlog::get("StarChips");

  MyTestHw hw;

  logger->debug("++ Make async");
  AsyncAccess::AsyncContext anon{hw, hw};

  logger->debug("++ Do async read");
  uint32_t rx_channel = 0;
  std::vector<AsyncAccess::AsyncReg> regs;

  for(uint32_t idx=0; idx<10; idx++) {
    regs.push_back
      (
       AsyncAccess::AsyncReg
       (anon,
        [&, idx](TxCore &) {
          hw.data.push_back({std::make_unique<RawData>(rx_channel, std::vector<uint32_t>{idx, idx*2})});
        },
        // Filter out everything except what we expect
        [idx](const RawData &d){ return d.getBuf()[0] == idx; },
        // Check filtered data is correct
        [idx](const RawData &d){ CHECK(d.getBuf()[1] == idx*2); }
        ));
  }

  logger->debug("++ Async read in progress");

  for(auto &r: regs) {
    r.result.get();
  }
  logger->debug("++ Finish async test");
}

TEST_CASE("AsyncReadRegisterMultipleChannels", "[Async]") {
  // For instance reads from multiple registers discriminate by address
  auto l = spdlog::get("StarChips");

  MyTestHw hw;

  logger->debug("++ Make async");
  AsyncAccess::AsyncContext anon{hw, hw};

  logger->debug("++ Do async read");
  std::vector<AsyncAccess::AsyncReg> regs;

  for(uint32_t idx=0; idx<10; idx++) {
    regs.push_back
      (
       AsyncAccess::AsyncReg
       (anon,
        [&, idx](TxCore &) {
          hw.data.push_back({std::make_unique<RawData>(idx, std::vector<uint32_t>{idx, idx*2})});
        },
        // Filter on rx channel
        [idx](const RawData &d){ return d.getAdr() == idx; },
        // Check filtered data is correct
        [idx](const RawData &d){
          CHECK(d.getBuf()[0] == idx);
          CHECK(d.getBuf()[1] == idx*2);
        }
        ));
  }

  logger->debug("++ Async read in progress");

  for(auto &r: regs) {
    r.result.get();
  }
  logger->debug("++ Finish async test");
}
