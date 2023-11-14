#include "catch.hpp"

#include "HistogramBase.h"
#include "Histo1d.h"
#include "Histo2d.h"

namespace tests {
namespace HistogramLoading {

// Things to be checked
struct HistoInfo {
  // Name
  std::string n;
  // Size
  size_t s;
  // Entry count
  size_t e;

  // Position is all that's saved from LoopStatus
  std::vector<unsigned> ls;

  // Axes
  std::string ax{"x"}, ay{"y"}, az{"z"};
};

void checkHisto1D(const Histo1d &hh, const HistoInfo &hi) {
  CAPTURE (hi.n, hi.s, hi.e, hi.ls);

  CHECK (hh.getName() == hi.n);
  CHECK (hh.size() == hi.s);
  CHECK (hh.getEntries() == hi.e);

  CHECK (hh.getXaxisTitle() == hi.ax);
  CHECK (hh.getYaxisTitle() == hi.ay);
  CHECK (hh.getZaxisTitle() == hi.az);

  if(!hi.ls.empty()) {
    auto &ls = hh.getStat();
    CHECK (ls.size() == hi.ls.size());
    for(size_t i = 0; i<ls.size(); i++) {
      CAPTURE (i);
      CHECK (ls.get(i) == hi.ls[i]);
    }
  } else {
    CHECK (hh.getStat().size() == hi.ls.size());
  }
}

void checkHisto2D(const Histo2d &hh, const HistoInfo &hi) {
  CAPTURE (hi.n, hi.s, hi.e, hi.ls);

  CHECK (hh.getName() == hi.n);
  CHECK (hh.size() == hi.s);
  CHECK (hh.getNumOfEntries() == hi.e);

  CHECK (hh.getXaxisTitle() == hi.ax);
  CHECK (hh.getYaxisTitle() == hi.ay);
  CHECK (hh.getZaxisTitle() == hi.az);

  if(!hi.ls.empty()) {
    auto &ls = hh.getStat();
    CHECK (ls.size() == hi.ls.size());
    for(size_t i = 0; i<ls.size(); i++) {
      CAPTURE (i);
      CHECK (ls.get(i) == hi.ls[i]);
    }
  } else {
    CHECK (hh.getStat().size() == hi.ls.size());
  }
}

// This is similar to procedure in libUtil tests, but don't have to specify
// types etc.
// 1. Check contents
// 2. Save to json
// 3. Read from json
// 4. Check contents of new histo
void testSaveLoad1D(const Histo1d &hh, const HistoInfo &hi) {
  checkHisto1D(hh, hi);

  json j;
  hh.toJson(j);

  // For debugging
  std::string json_output;
  j.dump(json_output);
  CAPTURE (json_output);

  std::unique_ptr<HistogramBase> out_histo;
  if(hi.ls.empty()) {
    out_histo = HistogramBase::fromJson(j);
  } else {
    // Make a template LoopStatus
    std::vector<LoopStyle> styleVec;
    std::vector<unsigned> statVec;

    for(size_t i=0; i<hi.ls.size(); i++) {
      styleVec.push_back(LoopStyle::LOOP_STYLE_NOP);
      statVec.push_back(0);
    }

    LoopStatus ls{std::move(statVec), styleVec};

    out_histo = HistogramBase::fromJson(j, ls);
  }

  Histo1d *out_histo1 = dynamic_cast<Histo1d*>(out_histo.get());

  // Should get a Histo1d out
  REQUIRE (out_histo1 != nullptr);

  // Check the loaded histogram looks the same
  checkHisto1D(*out_histo1, hi);
}

void testSaveLoad2D(const Histo2d &hh, const HistoInfo &hi) {
  checkHisto2D(hh, hi);

  json j;
  hh.toJson(j);

  // For debugging
  std::string json_output;
  j.dump(json_output);
  CAPTURE (json_output);

  std::unique_ptr<HistogramBase> out_histo;

  if(hi.ls.empty()) {
    out_histo = HistogramBase::fromJson(j);
  } else {
    // Make a template LoopStatus
    std::vector<LoopStyle> styleVec;
    std::vector<unsigned> statVec;

    for(size_t i=0; i<hi.ls.size(); i++) {
      styleVec.push_back(LoopStyle::LOOP_STYLE_NOP);
      statVec.push_back(0);
    }

    LoopStatus ls{std::move(statVec), styleVec};

    out_histo = HistogramBase::fromJson(j, ls);
  }

  auto out_histo2 = dynamic_cast<Histo2d*>(out_histo.get());

  // Should get a Histo2d out
  CHECK (out_histo2 != nullptr);

  // Check the loaded histogram looks the same
  checkHisto2D(*out_histo2, hi);
}

}} // Close namespace

using namespace tests::HistogramLoading;

TEST_CASE("Histogram_1D_loading", "[Histo1d][HistogramBase][HistogramLoad]") {
  std::unique_ptr<Histo1d> histo = std::make_unique<Histo1d>("TestHisto", 3, 0, 3);

  // NB Most of Histo1d::fromJson has been tested in libUtil
  // So this is mainly testing extra things (for now axes and loop status)
  HistoInfo info{"TestHisto", 3, 0, {}};

  SECTION("Default") {
  }

  SECTION("Small") {
    histo = std::make_unique<Histo1d>("TestHisto", 2, 0, 4);
    info.s = 2;

    histo->setXaxisTitle("XX");
    histo->setYaxisTitle("Counts");
    info.ax = "XX";
    info.ay = "Counts";
  }

  SECTION("SmallLoopStatus") {
    std::vector<LoopStyle> styleVec;
    styleVec.push_back(LoopStyle::LOOP_STYLE_DATA);
    styleVec.push_back(LoopStyle::LOOP_STYLE_PARAMETER);

    std::vector<unsigned> statVec{3, 2};

    LoopStatus ls{std::move(statVec), styleVec};

    CHECK (ls.get(0) == 3);
    CHECK (ls.get(1) == 2);

    histo = std::make_unique<Histo1d>("TestHisto", 4, 0, 4, ls);
    info.s = 4;
    info.ls = statVec;
  }

  testSaveLoad1D(*histo, info);
}

TEST_CASE("Histogram_2D_loading", "[Histo2d][HistogramBase][HistogramLoad]") {
  std::unique_ptr<Histo2d> histo = std::make_unique<Histo2d>("TestHisto2", 3, 0, 3, 3, 0, 3);

  HistoInfo info{"TestHisto2", 9, 0, {}};

  // NB Most of Histo2d::fromJson has been tested in libUtil
  // So this is mainly testing extras

  SECTION("Default") {
  }

  SECTION("Small") {
    histo = std::make_unique<Histo2d>("TestHisto2", 4, 0, 4, 2, 0, 2);
    info.s = 8;

    histo->setXaxisTitle("XX");
    histo->setYaxisTitle("YY");
    histo->setZaxisTitle("Counts");
    info.ax = "XX";
    info.ay = "YY";
    info.az = "Counts";
  }

  SECTION("SmallLoopStatus") {
    std::vector<LoopStyle> styleVec;
    styleVec.push_back(LoopStyle::LOOP_STYLE_DATA);
    styleVec.push_back(LoopStyle::LOOP_STYLE_PARAMETER);
    styleVec.push_back(LoopStyle::LOOP_STYLE_PARAMETER);

    std::vector<unsigned> statVec{4, 5, 6};

    LoopStatus ls{std::move(statVec), styleVec};

    histo = std::make_unique<Histo2d>("TestHisto2LS",
                                      4, 0, 4,
                                      6, 0, 6,
                                      ls);
    info.n = "TestHisto2LS";
    info.s = 24;
    info.ls = statVec;
  }

  testSaveLoad2D(*histo, info);
}
