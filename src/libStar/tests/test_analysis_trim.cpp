#include "catch.hpp"

#include "AllAnalyses.h"
#include "Bookkeeper.h"
#include "GraphErrors.h"
#include "Histo1d.h"
#include "JsonData.h"
#include "StarChips.h"
#include "ScanFactory.h"
#include "StarConstants.h"

#include "EmptyHw.h"

#include "logging.h"

namespace {
auto logger = logging::make_log("test_analysis_trim");
}

TEST_CASE("StarTrimDacAnalysis", "[Analysis][Star][Trim]") {

    // Need enough points that the fit has enough data for 4 params!
    std::vector<float> trim_points{0, 1, 2};
    std::vector<float> range_points{0, 1, 2};

    int n_injections = 50;

    json analysisCfg;

    int chip_count = 0;
    std::vector<int> abcIDs;
    std::vector<int> hccInputChannels;
    SECTION ("One Chip") {
      chip_count = 1;
      abcIDs = {0};
      hccInputChannels = {0};
    }

    SECTION ("Two Chips") {
      chip_count = 2;
      abcIDs = {3, 5};
      hccInputChannels = {2, 8};
    }

    // TODO: test more things? Add trigger loop to get trig count from

    unsigned trim_count = trim_points.size();
    unsigned range_count = range_points.size();

    CAPTURE (trim_count);
    CAPTURE (range_count);
    CAPTURE (chip_count);

    EmptyHw empty;
    Bookkeeper bookie(&empty, &empty);

    unsigned rx_channel = 0;
    auto frontEnd = std::make_unique<StarChips>(1, 1);
    frontEnd->setActive(true);
    for (size_t i = 0; i < chip_count; i++) {
      frontEnd->addABCchipID(abcIDs[i], hccInputChannels[i]);
    }
    int icMask = 0;
    for (int inputChannel : hccInputChannels) {
      icMask |= (1 << inputChannel);
    }
    frontEnd->hcc().setSubRegisterValue(HCCStarSubRegister::ICENABLE, icMask);
    bookie.addFe(std::move(frontEnd), rx_channel);

    auto starCfg = dynamic_cast<StarCfg*>(bookie.getFeCfg(rx_channel));
    std::vector<AbcCfg*> abcCfgs;
    for (int hccChannel : hccInputChannels) {
        abcCfgs.push_back(&starCfg->abcForInputChannel(hccChannel));
    }
    
    {
      // by default the trim DAC is 15
      for (auto &abcCfg : abcCfgs) {
        CAPTURE(abcCfg->getABCchipID());
        CHECK(abcCfg->getTrimDACRaw(0) == 15);
      }
    }

    // This is for one FE
    AnalysisProcessor analysis(rx_channel);

    int nCol = Star::StripsPerABCRow * chip_count;
    int nRow = Star::RowsPerABC;

    {
      auto ana = StdDict::getAnalysis("StarTrimDacAnalysis");

      REQUIRE (ana);

      ana->loadConfig(analysisCfg);

      ana->setConfig(bookie.getFeCfg(rx_channel));

      ana->setMapSize(nCol, nRow);

      analysis.addAlgorithm(std::move(ana));
    }

    // Need scan loops to lookup trigger loop
    ScanFactory scan(&bookie, nullptr);

    // Setup the loops that analysis needs to know about
    // Skip the threshold loop as that's already analysed by ScurveFitter
    {
      json scanCfg;
      scanCfg["scan"]["name"] = "TestTrimAnalysis";

      // Create Loop objects so they're available to analysis
      scanCfg["scan"]["loops"][0]["loopAction"] = "StdParameterLoop";
      scanCfg["scan"]["loops"][0]["config"]["min"] = 0;
      scanCfg["scan"]["loops"][0]["config"]["max"] = range_count - 1;
      scanCfg["scan"]["loops"][0]["config"]["step"] = 1;
      scanCfg["scan"]["loops"][0]["config"]["parameter"] = "ABCs_BTRANGE";

      scanCfg["scan"]["loops"][1]["loopAction"] = "StdParameterLoop";
      scanCfg["scan"]["loops"][1]["config"]["min"] = 0;
      scanCfg["scan"]["loops"][1]["config"]["max"] = trim_count - 1;
      scanCfg["scan"]["loops"][1]["config"]["step"] = 1;
      scanCfg["scan"]["loops"][1]["config"]["parameter"] = "TRIMs";

      scan.loadConfig(scanCfg);
    }

    ClipBoard<HistogramBase> input;
    ClipBoard<HistogramBase> output;

    analysis.connect(&scan, &input, &output, nullptr);

    analysis.init();
    analysis.run();

    for(unsigned ti=0; ti<trim_count; ti++) {
        for(unsigned ri=0; ri<range_count/2; ri++) {
            LoopStatus stat{{ti, ri}, {LOOP_STYLE_PARAMETER, LOOP_STYLE_PARAMETER}};
            auto hist = std::make_unique<Histo2d>("ThresholdMap",
                                                  nCol, 0.5, nCol + 0.5,
                                                  nRow, 0.5, nRow + 0.5, stat);

            CHECK(hist->getYbins() == nRow);
            CHECK(hist->getXbins() == nCol);

            int val = range_points[ri] + trim_points[ti];

            for(int c=0; c<nCol; c++) {
              for(int r=0; r<nRow; r++) {
                hist->fill(c + 1, r + 1, val);
              }
            }

            input.pushData(std::move(hist));

            if (!output.empty()) {
                logger->debug("Exit histo loop as have output to check");
                break;
            }
        }
    }

    input.finish();
    analysis.join();

    REQUIRE (!output.empty());

    int histo_count = 0;

    // Do some very basic checks on output
    while(!output.empty()) {
        std::unique_ptr<HistogramBase> result = output.popData();

        auto output_name = result->getName();

        CAPTURE (output_name);

        histo_count ++;

        {
            // Report info about any other data
            CAPTURE (result->getXaxisTitle());
            CAPTURE (result->getYaxisTitle());
            CAPTURE (result->getZaxisTitle());

            logger->trace("Extra histogram {}: {}", output_name,
                          [&]() -> std::string {
                json j; result->toJson(j);
                std::stringstream ss; ss << j; return ss.str(); }());

            // CHECK (false);
        }

        {
          // now the trim DAC is 0 given the dummy data used as input
          for (auto &abcCfg : abcCfgs) {
            CAPTURE(abcCfg->getABCchipID());
            REQUIRE(abcCfg->getTrimDACRaw(0) == 0);
          }
        }
    }

    CHECK (histo_count == 1 + chip_count * 5);
}
