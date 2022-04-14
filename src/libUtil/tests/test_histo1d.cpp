#include "catch.hpp"

#include "Histo1d.h"

struct HistoInfo {
  size_t s;
  size_t e;
  double m;
  double sd;
  double u;
  double o;
};

void checkHisto(const Histo1d &hh, const HistoInfo &hi) {
  REQUIRE (hh.size() == hi.s);
  REQUIRE (hh.getEntries() == hi.e);
  REQUIRE (hh.getMean() == hi.m);
  REQUIRE (hh.getStdDev() == hi.sd);
  REQUIRE (hh.getUnderflow() == hi.u);
  REQUIRE (hh.getOverflow() == hi.o);
}

void testSaveLoad(const Histo1d &hh, const HistoInfo &hi) {
  checkHisto(hh, hi);

  json j;
  hh.toJson(j);

  Histo1d out_histo{j["Name"], j["x"]["Bins"], j["x"]["Low"], j["x"]["High"]};

  out_histo.fromJson(j);

  // Check the loaded histogram looks the same
  checkHisto(out_histo, hi);
}

TEST_CASE("Histogram1dOK", "[Histo1d]") {
  Histo1d histo("TestHisto", 3, 0, 3);

  HistoInfo info{3, 0, 0.0, 0.0, 0.0, 0.0};

  SECTION("Default") {
  }

  SECTION("One Entry") {
    histo.fill(1.0);
    // info.e ++;
    info.m = 1.0;
  }

  SECTION("Two Entries") {
    histo.fill(1.0);
    histo.fill(2.0);
    // info.e += 2;
    info.m = 1.0;
  }

  SECTION("Overflow") {
    histo.fill(10.0);
    info.o = 1.0;
  }

  SECTION("Underflow") {
    histo.fill(-10.0);
    info.u = 1.0;
  }

  testSaveLoad(histo, info);
}
