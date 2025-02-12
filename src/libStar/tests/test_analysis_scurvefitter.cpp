#include "catch.hpp"

#include "AllAnalyses.h"
#include "AllChips.h"
#include "Bookkeeper.h"
#include "EmptyHw.h"
#include "EmptyFrontEnd.h"
#include "ScanFactory.h"

TEST_CASE("ScurveFitterAnalysis", "[Analysis][ScurveFitter]") {

    json analysisConfig;

    std::vector<unsigned> thresholds = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::vector<unsigned> counts;
    SECTION("Default") {
        counts = {0, 0, 0, 10, 25, 40, 50, 50, 50};
    }
    SECTION("Reverse") {
        counts = {50, 50, 50, 40, 25, 10, 0, 0, 0};
        analysisConfig["reverse"] = true;
    }

    unsigned channel = 0;

    EmptyHw empty;
    Bookkeeper bookie(&empty, &empty);

    auto frontEnd = StdDict::getFrontEnd("Star");
    frontEnd->setActive(true);
    bookie.addFe(std::move(frontEnd), channel);

    AnalysisProcessor processor(channel);

    int nCol = 2;
    int nRow = 2;

    {
        auto analysis = StdDict::getAnalysis("ScurveFitter");
        REQUIRE(analysis);

        analysis->loadConfig(analysisConfig);
        analysis->setMapSize(nCol, nRow);
        analysis->setConfig(bookie.getFeCfg(channel));

        processor.addAlgorithm(std::move(analysis));
    }

    ScanFactory scan(&bookie, nullptr);

    {
      json scanCfg;
      scanCfg["scan"]["name"] = "TestScurveFitterAnalysis";

      scanCfg["scan"]["loops"][0]["loopAction"] = "StdParameterLoop";
      scanCfg["scan"]["loops"][0]["config"]["min"] = 1;
      scanCfg["scan"]["loops"][0]["config"]["max"] = thresholds.size()-1;
      scanCfg["scan"]["loops"][0]["config"]["step"] = 1;

      scan.loadConfig(scanCfg);
    }

    ClipBoard<HistogramBase> input;
    ClipBoard<HistogramBase> output;

    processor.connect(&scan, &input, &output, nullptr);

    processor.init();
    processor.run();

    for (unsigned i = 0; i < thresholds.size(); i++) {
        LoopStatus stat{{i}, {LOOP_STYLE_PARAMETER}};
        auto hist = std::make_unique<Histo2d>("OccupancyMap",
                                                nCol, 0.5, nCol+0.5,
                                                nRow, 0.5, nRow+0.5,
                                                stat);
        
        for (int c=0; c<nCol; c++) {
            for (int r=0; r<nRow; r++) {
                hist->fill(c+1, r+1, counts[i]);
                CHECK(hist->getBin(hist->binNum(c+1, r+1)) == counts[i]);
            }
        }

        input.pushData(std::move(hist));
    }

    input.finish();
    processor.join();

    REQUIRE(!output.empty());

    int histo_count = 0;
    while (!output.empty()) {
        std::unique_ptr<HistogramBase> result = output.popData();
        histo_count++;

        auto output_name = result->getName();
        auto hh = dynamic_cast<Histo2d*>(result.get());
        
        for(int r=0; r<nRow; r++) {
            for(int c=0; c<nCol; c++) {
                CAPTURE(c);
                CAPTURE(r);

                if (output_name.find("ThresholdMap") != std::string::npos) {
                    REQUIRE(hh->getBin(hh->binNum(c+1,r+1)) == 4);
                } else if (output_name.find("Chi2Map") != std::string::npos) {
                    REQUIRE(hh->getBin(hh->binNum(c+1,r+1)) < 1);
                } else if (output_name.find("StatusMap") != std::string::npos) {
                    REQUIRE(hh->getBin(hh->binNum(c+1,r+1)) > 0);
                }

            }
        }
    }
}