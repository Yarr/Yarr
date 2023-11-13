#include "catch.hpp"

#include "Histo1d.h"
#include "Histo2d.h"

struct HistoInfo {
  size_t s;
  size_t e;
  double m;
  double sd;
  double u;
  double o;
  double mx;
  double mn;
};

void checkHisto(const Histo2d &hh, const HistoInfo &hi) {
  CAPTURE (hi.s, hi.e, hi.m, hi.sd, hi.u, hi.o, hi.mx, hi.mn);

  CHECK (hh.size() == hi.s);
  CHECK (hh.numOfEntries() == hi.e);
  CHECK (hh.getNumOfEntries() == hi.e);
  CHECK (hh.getMean() == Catch::Approx(hi.m));
  CHECK (hh.getStdDev() == Catch::Approx(hi.sd));
  CHECK (hh.getUnderflow() == hi.u);
  CHECK (hh.getOverflow() == hi.o);

  CHECK (hh.getMax() == hi.mx);
  CHECK (hh.getMin() == hi.mn);
}

// For a given histogram:
// 1. Check contents
// 2. Save to json
// 3. Read from json
// 4. Check contents of new histo
// 5. Write new histo to json and compare with previous
void testSaveLoad(const Histo2d &hh, const HistoInfo &hi) {
  checkHisto(hh, hi);

  json j;
  hh.toJson(j);

  Histo2d out_histo{j["Name"],
                    j["x"]["Bins"], j["x"]["Low"], j["x"]["High"],
                    j["y"]["Bins"], j["y"]["Low"], j["y"]["High"]};

  out_histo.fromJson(j);

  std::string json_output;
  j.dump(json_output);

  CAPTURE (json_output);

  // Check the loaded histogram looks the same
  checkHisto(out_histo, hi);

  // Belt and braces, if we write to json again, we should get the same result...
  json j2;
  out_histo.toJson(j2);

  std::string json2_output;
  j2.dump(json2_output);

  CHECK (json_output == json2_output);
}

TEST_CASE("Histogram2dOK", "[Histo2d]") {
  Histo2d histo("TestHisto", 3, 0, 3, 3, 0, 3);

  HistoInfo info{9, 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

  SECTION("Default") {
  }

  SECTION("One Entry") {
    histo.fill(1.0, 1.0);
    info.e ++;
    info.m = 1.0;
    info.mx = 1.0;
  }

  SECTION("Two Entries") {
    histo.fill(1.0, 1.0);
    histo.fill(2.0, 1.0);
    info.e += 2;
    info.m = 1.0;
    info.sd = 0.0;
    info.mx = 1.0;
  }

  SECTION("Two unequal entries") {
    histo.fill(1.0, 1.0);
    histo.fill(2.0, 2.0, 4.0);
    info.e += 2;
    info.m = 2.5;
    info.sd = sqrt(1.5 * 1.5 + 1.5 * 1.5);
    info.mx = 4.0;
  }

  // TODO: setBin doesn't add to entries, or max?

  SECTION("Two Entries via add") {
    histo.fill(1.0, 1.0);
    Histo2d histo_to_add("TestHistoAdd", 3, 0, 3, 3, 0, 3);
    histo_to_add.fill(2.0, 2.0, 3.0);
    histo.add(histo_to_add);
    info.e += 2;
    info.m = 2.0;
    info.sd = sqrt(2.0);
    info.mx = 3.0;
  }

  SECTION("Overflow") {
    histo.fill(10.0, 1.0);
    histo.fill(1.0, 10.0, 2.0);
    histo.fill(10.0, 10.0);

    info.e = 3;
    info.o = 4.0;
  }

  SECTION("Underflow") {
    histo.fill(-10.0, 1.0);
    histo.fill(1.0, -10.0, 2.0);
    histo.fill(-10.0, -10.0);
    // Underflow has precedence over overflow
    histo.fill(10.0, -10.0);
    histo.fill(-10.0, 10.0, 3.0);

    info.e = 5;
    info.u = 8.0;
  }

  testSaveLoad(histo, info);
}

// Check that the right bins are being selected for the profile
TEST_CASE("Histogram2dProfile", "[Histo2d]") {
  size_t check_size = 3;

  auto histo = std::make_unique<Histo2d>("Histo", 3, 0, 3, 3, 0, 3);

  CHECK (histo->getXbinWidth() == 1.0);
  CHECK (histo->getYbinWidth() == 1.0);

  double check = 0.0;

  // Empty histogram is easy
  SECTION("Zero") { }

  SECTION("One Third") {
    check = 1.0/3.0;
    histo->fill(0, 0, 1.0);
    histo->fill(0, 1, 2.0);
  }

  SECTION("Bigger") {
    check_size = 128;
    histo = std::make_unique<Histo2d>("Histo", check_size, 0, check_size, check_size, 0, check_size);
    check = 1.0;
    for(int i=0; i<128; i++) {
      histo->fill(i, 0, 1.0);
      histo->fill(i, 1, 2.0);
    }
  }

  SECTION("One") {
    check = 1.0;
    histo->fill(0, 0, 1.0);
    histo->fill(1, 0, 1.0);
    histo->fill(2, 0, 1.0);
    histo->fill(0, 1, 2.0);
    histo->fill(1, 1, 2.0);
    histo->fill(2, 1, 2.0);
  }

  auto p = histo->profileY();

  CHECK (p->size() == check_size);
  CHECK (p->getBin(0) == check);
  CHECK (p->getBin(1) == check * 2);
}
