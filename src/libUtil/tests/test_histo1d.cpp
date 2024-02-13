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
  CAPTURE (hi.s, hi.e, hi.m, hi.sd, hi.u, hi.o);

  CHECK (hh.size() == hi.s);
  CHECK (hh.getEntries() == hi.e);
  CHECK (hh.getMean() == Catch::Approx(hi.m));
  CHECK (hh.getStdDev() == Catch::Approx(hi.sd));
  CHECK (hh.getUnderflow() == hi.u);
  CHECK (hh.getOverflow() == hi.o);
}

// For a given histogram:
// 1. Check contents
// 2. Save to json
// 3. Read from json
// 4. Check contents of new histo
void testSaveLoad(const Histo1d &hh, const HistoInfo &hi) {
  checkHisto(hh, hi);

  json j;
  hh.toJson(j);

  Histo1d out_histo{j["Name"], j["x"]["Bins"], j["x"]["Low"], j["x"]["High"]};

  out_histo.fromJson(j);

  std::string json_output;
  j.dump(json_output);

  CAPTURE (json_output);

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
    info.e ++;
    info.m = 1.5;
  }

  SECTION("Two Entries") {
    histo.fill(1.0);
    histo.fill(2.0);
    info.e += 2;
    info.m = 2.0;
    info.sd = 0.5;
  }

  SECTION("Two unequal entries") {
    histo.fill(1.0);
    histo.setBin(2, 4.0);
    info.e += 2;
    info.m = 2.3;
    info.sd = 0.4;
  }

  SECTION("Two Entries via add") {
    histo.fill(1.0);
    Histo1d histo_to_add("TestHistoAdd", 3, 0, 3);
    histo_to_add.fill(2.0);
    histo.add(histo_to_add);
    info.e += 2;
    info.m = 2.0;
    info.sd = 0.5;
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
