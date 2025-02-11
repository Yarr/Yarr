#include <cmath>

#include "catch.hpp"
#include "logging.h"

#include "AllAnalyses.h"
#include "Bookkeeper.h"
#include "EmptyHw.h"
#include "ScanFactory.h"
#include "StarChips.h"

namespace {
auto logger = logging::make_log("test_analysis_npointgain");
}

TEST_CASE("StarNPointGainAnalysis", "[Star][Analysis][NPointGain]") {
  /*
   * libYarr test_npointgain.cpp already tests the main functionality
   * of the NPointGain analysis. This test focuses on the Star-specific
   * averaging and configuration writing components.
   */

  json analysisConfig;

  std::vector<unsigned> injections = {0, 1};
  std::vector<double> evenResponses, oddResponses; // two sets for averaging
  std::vector<double> noises;

  auto linear = [](double x, const double *par) { return par[0] + par[1] * x; };

  analysisConfig["fitFunction"] = "linear";
  double evenParams[] = {1, 2}; // values are specific to
  double oddParams[] = {0, 4};  // the desired fit params below
  std::vector<double> desiredFitParams = {0.25, 76.4};
  std::vector<double> tolerances = {0.01, 0.1};

  for (const auto &inj : injections) {
    evenResponses.push_back(linear(inj, evenParams));
    oddResponses.push_back(linear(inj, oddParams));
    noises.push_back(inj);
    CAPTURE(evenResponses);
    CAPTURE(oddResponses);
    CAPTURE(noises);
  }

  analysisConfig["parametersOfInterest"] = {"BCAL"};

  EmptyHw empty;
  Bookkeeper bookie(&empty, &empty);

  unsigned channel = 0;
  auto frontEnd =
      std::make_unique<StarChips>(1, 1); // vH1A1 but makes no real difference
  frontEnd->setActive(true);
  bookie.addFe(std::move(frontEnd), channel);

  AnalysisProcessor processor(channel);

  int nCol = 128;
  int nRow = 2;

  {
    auto analysis = StdDict::getAnalysis("StarNPointGainAnalysis");
    REQUIRE(analysis);

    analysis->loadConfig(analysisConfig);
    analysis->setMapSize(nCol, nRow);
    analysis->setConfig(bookie.getFeCfg(channel));

    processor.addAlgorithm(std::move(analysis));
  }

  ScanFactory scan(&bookie, nullptr);

  {
    json scanCfg;
    scanCfg["scan"]["name"] = "TestStarNPointGainAnalysis";

    scanCfg["scan"]["loops"][0]["loopAction"] = "StdParameterLoop";
    scanCfg["scan"]["loops"][0]["config"]["min"] = 0;
    scanCfg["scan"]["loops"][0]["config"]["max"] = 2;
    scanCfg["scan"]["loops"][0]["config"]["step"] = 1;
    scanCfg["scan"]["loops"][0]["config"]["parameter"] = "BCAL";

    scan.loadConfig(scanCfg);
  }

  ClipBoard<HistogramBase> input;
  ClipBoard<HistogramBase> output;

  processor.connect(&scan, &input, &output, nullptr);

  processor.init();
  processor.run();

  for (unsigned i = 0; i < injections.size(); i++) {
    LoopStatus stat{{i}, {LOOP_STYLE_PARAMETER}};
    auto thresholdHist = std::make_unique<Histo2d>(
        "ThresholdMap", nCol, -0.5, nCol - 0.5, nRow, -0.5, nRow - 0.5, stat);
    auto noiseHist = std::make_unique<Histo2d>(
        "NoiseMap", nCol, -0.5, nCol - 0.5, nRow, -0.5, nRow - 0.5, stat);

    for (int c = 0; c < nCol; c++) {
      for (int r = 0; r < nRow; r++) {
        float response = c % 2 == 0 ? evenResponses[i] : oddResponses[i];
        thresholdHist->fill(c, r, response);
        CHECK(std::abs(thresholdHist->getBin(thresholdHist->binNum(c, r)) - response) < 0.01);
        noiseHist->fill(c, r, noises[i]);
        CHECK(std::abs(noiseHist->getBin(noiseHist->binNum(c, r)) - noises[i]) < 0.01);
      }
    }

    input.pushData(std::move(thresholdHist));
    input.pushData(std::move(noiseHist));
  }

  input.finish();
  processor.join();

  REQUIRE(!output.empty());

  auto *feAfter = dynamic_cast<StarChips *>(bookie.getFe(channel));
  auto &ctAfter = feAfter->getStarConversion();
  CHECK(ctAfter.getResponseFunctionName() == "linear");

  // check the StarConversionTools object directly
  unsigned nABCs = nCol / 128;
  for (unsigned abc = 0; abc < nABCs; abc++) {
    auto [name, params] = ctAfter.getResponseParameters(abc);
    CHECK(name == "linear");
    REQUIRE(params.size() == desiredFitParams.size());
    CAPTURE(abc);
    for (unsigned i = 0; i < params.size(); i++) {
      CAPTURE(i);
      CAPTURE(params[i]);
      CHECK(std::abs(params[i] - desiredFitParams[i]) < tolerances[i]);
    }
  }

  // check the configuration after it's written to json
  json configAfter = json::object();
  feAfter->writeConfig(configAfter);
  REQUIRE(configAfter.contains("ABCs"));
  REQUIRE(configAfter["ABCs"].contains("Parameters"));
  REQUIRE(configAfter["ABCs"]["Parameters"].contains("ResponseFitFunction"));
  CHECK(configAfter["ABCs"]["Parameters"]["ResponseFitFunction"] == "linear");
  REQUIRE(configAfter["ABCs"]["Parameters"].contains("ResponseFitParams"));

  std::vector<std::vector<double>> fitParams =
      configAfter["ABCs"]["Parameters"]["ResponseFitParams"];
  CHECK(fitParams.size() == nABCs);
  for (unsigned abc = 0; abc < fitParams.size(); abc++) {
    CAPTURE(abc);
    for (unsigned i = 0; i < fitParams[abc].size(); i++) {
      CAPTURE(fitParams[abc][i]);
      CHECK(std::abs(fitParams[abc][i] - desiredFitParams[i]) < tolerances[i]);
    }
  }
}