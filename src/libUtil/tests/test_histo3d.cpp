#include "catch.hpp"

#include "Histo1d.h"
#include "Histo3d.h"
#include "Histo3d.h"

template<typename DataT>
struct HistoInfo {
  size_t s; // size
  size_t e; // entries
  double m; // mean
  double sd; // stdev
  DataT u; // underflow
  DataT o; // overflow
  DataT mx; // max
  DataT mn; // min
};

template<typename DataT>
void checkHisto(const Histo3dT<DataT> &hh, const HistoInfo<DataT> &hi) {
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
template<typename DataT>
void testSaveLoad(const Histo3dT<DataT> &hh, const HistoInfo<DataT> &hi) {
  checkHisto(hh, hi);

  json j;
  hh.toJson(j);

  Histo3dT<DataT> out_histo{j["Name"],
                    j["x"]["Bins"], j["x"]["Low"], j["x"]["High"],
                    j["y"]["Bins"], j["y"]["Low"], j["y"]["High"],
                    j["z"]["Bins"], j["z"]["Low"], j["z"]["High"]};

  out_histo.fromJson(j);

  std::string json_output = j.dump();

  CAPTURE (json_output);

  // Check the loaded histogram looks the same
  checkHisto(out_histo, hi);

  // Belt and braces, if we write to json again, we should get the same result...
  json j2;
  out_histo.toJson(j2);

  std::string json2_output = j2.dump();

  CHECK (json_output == json2_output);
}

TEST_CASE("Histogram3dOK", "[Histo3d]") {
  Histo3dT<float> histo("TestHisto", 3, 0, 3, 3, 0, 3, 3, 0, 3);

  HistoInfo<float> info{27, 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

  SECTION("Default") {
  }

  SECTION("One Entry") {
    histo.fill(1.0, 1.0, 1.0);
    info.e ++;
    info.m = 1.0;
    info.mx = 1.0;
  }

  SECTION("Two Entries") {
    histo.fill(1.0, 1.0, 1.0);
    histo.fill(2.0, 1.0, 1.0);
    info.e += 2;
    info.m = 1.0;
    info.sd = 0.0;
    info.mx = 1.0;
  }

  SECTION("Two unequal entries") {
    histo.fill(1.0, 1.0, 0.0);
    histo.fill(2.0, 2.0, 0.0, 4.0);
    info.e += 2;
    info.m = 2.5;
    info.sd = sqrt(1.5 * 1.5 + 1.5 * 1.5);
    info.mx = 4.0;
  }

  // TODO: setBin doesn't add to entries, or max?

  SECTION("Two Entries via add") {
    histo.fill(1.0, 1.0, 1.0);
    Histo3dT<float> histo_to_add("TestHistoAdd", 3, 0, 3, 3, 0, 3, 3, 0, 3);
    histo_to_add.fill(2.0, 2.0, 2.0, 3.0);
    histo.add(histo_to_add);
    info.e += 2;
    info.m = 2.0;
    info.sd = sqrt(2.0);
    info.mx = 3.0;
  }

  SECTION("Overflow") {
    histo.fill(10.0, 1.0, 1.0);
    histo.fill(1.0, 10.0, 1.0, 2.0);
    histo.fill(10.0, 10.0, 10.0);

    info.e = 3;
    info.o = 4.0;
  }

  SECTION("Underflow") {
    histo.fill(-10.0, 1.0, 1.0);
    histo.fill(1.0, -10.0, 1.0, 2.0);
    histo.fill(-10.0, -10.0, -10.0);
    // Underflow has precedence over overflow
    histo.fill(10.0, -10.0, 10.0);
    histo.fill(-10.0, 10.0, 10.0, 3.0);

    info.e = 5;
    info.u = 8.0;
  }

  testSaveLoad(histo, info);
}

TEST_CASE("Histogram3dUint16OK", "[Histo3d]") {
  Histo3dT<uint16_t> histo("TestHisto", 3, 0, 3, 3, 0, 3, 3, 0, 3);

  HistoInfo<uint16_t> info{27, 0, 0.0, 0.0, 0, 0, 0, 0};

  SECTION("Default") {
  }

  SECTION("One Entry") {
    histo.fill(1, 1, 1);
    info.e ++;
    info.m = 1;
    info.mx = 1;
  }

  SECTION("Two Entries") {
    histo.fill(1, 1, 1);
    histo.fill(2, 1, 1);
    info.e += 2;
    info.m = 1;
    info.sd = 0;
    info.mx = 1;
  }

  SECTION("Two unequal entries") {
    histo.fill(1, 1, 0);
    histo.fill(2, 2, 0, 4);
    info.e += 2;
    info.m = 2.5;
    info.sd = sqrt(1.5 * 1.5 + 1.5 * 1.5);
    info.mx = 4;
  }

  // TODO: setBin doesn't add to entries, or max?

  SECTION("Two Entries via add") {
    histo.fill(1, 1, 1);
    Histo3dT<uint16_t> histo_to_add("TestHistoAdd", 3, 0, 3, 3, 0, 3, 3, 0, 3);
    histo_to_add.fill(2, 2, 2, 3);
    histo.add(histo_to_add);
    info.e += 2;
    info.m = 2;
    info.sd = sqrt(2);
    info.mx = 3;
  }

  SECTION("Overflow") {
    histo.fill(10, 1, 1);
    histo.fill(1, 10, 1, 2);
    histo.fill(10, 10, 10);

    info.e = 3;
    info.o = 4;
  }

  SECTION("Underflow") {
    histo.fill(-10, 1, 1);
    histo.fill(1, -10, 1, 2);
    histo.fill(-10, -10, -10);
    // Underflow has precedence over overflow
    histo.fill(10, -10, 10);
    histo.fill(-10, 10, 10, 3);

    info.e = 5;
    info.u = 8;
  }

  testSaveLoad(histo, info);
}
