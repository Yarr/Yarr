#include <cmath>

#include "catch.hpp"
#include "logging.h"

#include "AllAnalyses.h"
#include "Bookkeeper.h"
#include "EmptyHw.h"
#include "Histo3d.h"
#include "ScanFactory.h"
#include "StarChips.h"
#include "StarConstants.h"

namespace {
auto logger = logging::make_log("test_analysis_npointgain");
}

namespace {
auto BVTtomV = [](double x) { return 2.7264 * x + 1.041; };
auto BCALtofC = [](double x) { return x * (10. / (1 << 9)); };
} // namespace

TEST_CASE("StarNPointGainAnalysis", "[Star][Analysis][NPointGain]") {
  /*
   * libYarr test_npointgain.cpp already tests the main functionality
   * of the NPointGain analysis. This test focuses on the Star-specific
   * averaging, configuration writing, and noise unit conversion.
   */

  json analysisConfig;

  std::vector<unsigned> injections = {0, 1};
  // create two sets of response values for averaging test
  std::vector<double> evenResponses, oddResponses;
  std::vector<double> noises;

  auto linear = [](double x, const double *par) { return par[0] + par[1] * x; };

  analysisConfig["fitFunction"] = "linear";
  double evenParams[] = {2, 4};
  double oddParams[] = {2, 3};

  // apply basic BVT->mV (and BVT/BCAL -> mV/fC slope) conversion
  // for the expected {2, 3.5} fit params
  std::vector<double> desiredFitParams = {BVTtomV(2),
                                          BVTtomV(3.12) / BCALtofC(1)};

  for (const auto &inj : injections) {
    evenResponses.push_back(linear(inj, evenParams));
    oddResponses.push_back(linear(inj, oddParams));
    noises.push_back(inj + 1);
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

  int nCol = Star::StripsPerABCRow;
  int nRow = Star::RowsPerABC;

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
    LoopStatus stat{{injections[i]}, {LOOP_STYLE_PARAMETER}};
    auto thresholdHist = std::make_unique<Histo2d>(
        "ThresholdMap", nCol, 0.5, nCol + 0.5, nRow, 0.5, nRow + 0.5, stat);
    auto noiseHist = std::make_unique<Histo2d>(
        "NoiseMap", nCol, 0.5, nCol + 0.5, nRow, 0.5, nRow + 0.5, stat);

    for (int c = 0; c < nCol; c++) {
      for (int r = 0; r < nRow; r++) {
        float response = c % 2 == 0 ? evenResponses[i] : oddResponses[i];
        thresholdHist->fill(c + 1, r + 1, response);
        CHECK_THAT(thresholdHist->getBin(thresholdHist->binNum(c + 1, r + 1)),
                   Catch::Matchers::WithinAbs(response, 0.01));
        noiseHist->fill(c + 1, r + 1, noises[i]);
        CHECK_THAT(noiseHist->getBin(noiseHist->binNum(c + 1, r + 1)),
                   Catch::Matchers::WithinAbs(noises[i], 0.01));
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
  unsigned nABCs = nCol / Star::StripsPerABCRow;
  for (unsigned abc = 0; abc < nABCs; abc++) {
    auto [name, params] = ctAfter.getResponseParameters(abc);
    CHECK(name == "linear");
    REQUIRE(params.size() == desiredFitParams.size());
    CAPTURE(abc);
    for (unsigned i = 0; i < params.size(); i++) {
      CAPTURE(i);
      CAPTURE(params[i]);
      REQUIRE_THAT(params[i],
                   Catch::Matchers::WithinRel(desiredFitParams[i], 0.01));
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
      REQUIRE_THAT(fitParams[abc][i],
                   Catch::Matchers::WithinRel(desiredFitParams[i], 0.01));
    }
  }

  unsigned histoCount = 0;
  while (!output.empty()) {
    histoCount++;

    std::unique_ptr<HistogramBase> result = output.popData();
    auto outputName = result->getName();
    CAPTURE(outputName);

    if (outputName == "InputNoise") {
      auto h = dynamic_cast<Histo3dT<float> *>(result.get());
      REQUIRE(h);

      REQUIRE(h->size() == nCol * nRow * injections.size());
      for (unsigned c = 0; c < 1; c++) {
        CAPTURE(c);
        for (unsigned r = 0; r < 1; r++) {
          CAPTURE(r);
          for (unsigned i = 0; i < h->getZbins(); i++) {
            CAPTURE(i);
            CAPTURE(h->binNum(c, r, i));
            CAPTURE(h->getBin(h->binNum(c, r, i)));

            // get gain for our row and convert units
            double gainPreConv = (c % 2 == 0) ? evenParams[1] : oddParams[1];
            CAPTURE(gainPreConv);
            double gain = (BVTtomV(gainPreConv) / BCALtofC(1));
            CAPTURE(gain);

            double expected = (BVTtomV(noises[i]) / gain) * 6250; // fC to ENC
            CAPTURE(BVTtomV(noises[i]));
            CAPTURE(expected);

            REQUIRE_THAT(h->getBin(h->binNum(c, r, i)),
                         Catch::Matchers::WithinRel(expected, 0.15));
          }
        }
      }
    }
  }

  REQUIRE(histoCount == 6);
}