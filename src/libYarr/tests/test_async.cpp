#include "catch.hpp"

#include "AsyncReg.h"

#include "EmptyHw.h"
#include "EmptyFrontEnd.h"
#include "logging.h"

TEST_CASE("AsyncReadRegister", "[Async]") {
  auto l = spdlog::get("StarChips");

  EmptyHw hw;

  ReceiveRegisterScheduler sched(hw);

  uint32_t channel = 2;
  sched.readRegister(channel, [&](uint32_t reg_val) -> AsyncReply<uint32_t>
		     {
		       l << "Report reg data: " << reg_val << std::endl;
		     }
		     );
  sched.run();
}
