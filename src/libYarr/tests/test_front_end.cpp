#include "catch.hpp"

#include "AllChips.h"

TEST_CASE("FrontEndsValid", "[frontend]") {
  auto fe_list = StdDict::listFrontEnds();

  for(auto &fe_name: fe_list) {
    CAPTURE (fe_name);
    CHECK (StdDict::getFrontEnd(fe_name) != nullptr);
  }
}
